/* 
 * new moused
 *
 * $Id$
 */

#include <sys/cdefs.h>
__FBSDID("Happy Cakes!");

#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/select.h>

/* sysmouse stuff */
#include <sys/mouse.h>
#include <sys/consio.h>

#include "moused.h"

static int verbose = 0;

static void logmsg(int log_pri, int errnum, const char *fmt, ...);
static int moused(int argc, char **argv);

/* Load a module into memory (dlopen, etc) */
static void loadmodule(char *path);

/* Modules will call this function when the mouse moves/changes */
static int update(mouse_info_t *delta);

/* Filter the input somehow? */
static void filter(mouse_info_t *delta, int noop);

static void packethandler_main();
void dumpstate(mouse_info_t *delta);

#define debug(level, fmt, args...) do {                        \
	if (level <= verbose)                                       \
		warnx(fmt, ##args);                                      \
} while (0)

#define MOUSED_OPTIONS "d:m:f:"

static rodent_t rodent = {
	.device = NULL,
	.modulename = NULL,
	.configfile = "/etc/moused.conf",
	.cfd = -1,
	.mfd = -1,
	.update = update, /* Modules call moused's update(...) to talk to sysmouse */
	.logmsg = logmsg, /* Modules call moused's logmsg(...) log stuff */

	.init = NULL,
	.probe = NULL,
	.run = NULL,

	.handler = NULL,
};

int main(int argc, char **argv) {
	int c;

	while (-1 != (c = getopt(argc, argv, MOUSED_OPTIONS))) {
		switch (c) {
			case 'd':
				rodent.device = optarg; break;
			case 'm':
				rodent.modulename = optarg; break;
			case 'f':
				rodent.configfile = optarg; break;
		}
	}

	argc -= optind;
	argv += optind;

	if (NULL == rodent.device)
		rodent.device = "/dev/psm0";

	moused(argc, argv);

	return 0;
}

int moused(int argc, char **argv) {
	struct mouse_info mouse;

	if (-1 == (rodent.cfd = open("/dev/consolectl", O_RDWR, 0))) {
		logfatal(1, "Unable to open /dev/consolectl");
	}

	if (NULL != rodent.modulename) {
		char path[256];
		snprintf(path, 256, "modules/%s/libmoused_%s.so", 
					rodent.modulename, rodent.modulename);
		printf("Path: %s\n", path);
		loadmodule(path);

		if (NULL != rodent.init) {
			/* Reset getopt stuffs (do it in case init() likes to use getopt) */
			optreset = 1;
			optind = 0;
			rodent.init(&rodent, argc, argv);
		}

		if (NULL == rodent.probe)
			logfatal(1, "Module '%s' has no PROBE function?!", rodent.modulename);

		/* Check if this module is the right module for this mouse device */
		if (rodent.probe(&rodent) == MODULE_PROBE_SUCCESS) {

			/* Try the RUN method first */
			//if (NULL != rodent.run)
				//rodent.run(&rodent);
			/* else */ if (NULL != rodent.handler)
				packethandler_main();
			else 
				logfatal(1, "Module '%s' has no RUN function?!", rodent.modulename);

			/* If we _EVER_ get here, something hs wrong */
			warnx("Arrived at unexpected location (RUN or PACKETHANDLER method died?)");
		}
	} else {
		warnx("No module selected.");
		exit(1);
	}
}

static void logmsg(int log_pri, int errnum, const char *fmt, ...) {
	va_list ap;
	char buf[256];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (errnum) {
		strlcat(buf, ": ", sizeof(buf));
		strlcat(buf, strerror(errnum), sizeof(buf));
	}

	/* Syslog if we're in the background? */
	warnx("%s", buf);
}

static void loadmodule(char *path) {
	void *handle;

	if (NULL == (handle = dlopen(path, RTLD_LAZY))) {
		logfatal(1, "Unable open module '%s'.", rodent.modulename);
	}

	rodent.init = dlsym(handle, "init");
	rodent.probe = dlsym(handle, "probe");
	rodent.run = dlsym(handle, "run");
	rodent.handler = dlsym(handle, "handler");
	rodent.handler_init = dlsym(handle, "handler_init");
}

static int update(mouse_info_t *delta) {
	mouse_info_t mydelta;

	/* Store state and copy delta into mydelta if this isn't a no-op */
	if (NULL != delta) {
		memcpy(&mydelta, delta, sizeof(mouse_info_t));
		memcpy(&(rodent.state), delta, sizeof(mouse_info_t));
	}
	else {
		memset(&mydelta, 0, sizeof(mouse_info_t));
	}

	filter(&mydelta, (NULL == delta) ? 1 : 0);
	return ioctl(rodent.cfd, CONS_MOUSECTL, &mydelta);
}

static int state = 0;
static int distance = 0;
#define NORMAL 0
#define SCROLLING 1
#define SCROLLREADY 2
#define SCROLLTIMEOUT 3

/* 
 * This is all the code a filter should have to be 
 * How should this be implemented?
 */
static void filter(mouse_info_t *delta, int noop) {
	if (noop == 1) {
		if (state == SCROLLREADY) {
			warnx("noop");
			memcpy(delta, &(rodent.state), sizeof(mouse_info_t));
			dumpstate(delta);
			state = SCROLLTIMEOUT;
		}
	} else {
		if (delta->u.data.buttons & (1<<1)) {
			if (state == SCROLLTIMEOUT)
				return;

			state = SCROLLREADY;

			/* Middle mouse is held */
			if (delta->u.data.x != 0 || delta->u.data.y != 0) {
				if (state == SCROLLREADY)
					state = SCROLLING;

				distance += delta->u.data.y;

#define THRESHOLD 3

				if (distance > THRESHOLD) {
					delta->u.data.z = 1;
					distance = 0;
				} else if (distance < -THRESHOLD) {
					delta->u.data.z = -1;
					distance = 0;
				}

				//if (delta->u.data.x > 4) {
					//delta->u.data.z += 2;
				//} else if (delta->u.data.x < 4) {
					//delta->u.data.z += -2;
				//}

				/* Do not move! */
				delta->u.data.x = delta->u.data.y = 0;
			}

			/* Don't click middle */
			delta->u.data.buttons &= ~(1<<1);
		} else {
			state = NORMAL;
		}
	}
}

static void packethandler_main() {
	fd_set fds;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 100000; /* 20mec */

	if (NULL != rodent.handler_init)
		rodent.handler_init(&rodent);

	if (rodent.mfd == -1) {
		rodent.mfd = open(rodent.device, O_RDWR);
		if (rodent.mfd == -1) 
			logfatal(1, "Failed trying to open '%s'", rodent.device);
	}

	FD_ZERO(&fds);
	FD_SET(rodent.mfd, &fds);
	
	for (;;) {
		int c;

		c = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
		if (c < 0)
			logfatal(1, "Error with select call");
		else if (c > 0)
			rodent.handler(&rodent);
		else 
			rodent.update(NULL); /* TIMEOUT HAPPENED */

		FD_SET(rodent.mfd, &fds);
	}
}

void dumpstate(mouse_info_t *delta) {
	warnx("(%d,%d) z:%d b:%08x", delta->u.data.x, delta->u.data.y,
			delta->u.data.z, delta->u.data.buttons);
}
