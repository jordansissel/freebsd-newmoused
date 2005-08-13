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
#include <syslog.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

/* sysmouse stuff */
#include <sys/mouse.h>
#include <sys/consio.h>

/* Don't include things needed by module libraries */
#define THIS_IS_MOUSED 1

#include "moused.h"

static int verbose = 0;

static void logmsg(int level, int errnum, const char *fmt, ...);
static int moused(int argc, char **argv);
static void loadmodule(char *path);
static void update(struct mouse_info *delta);

#define debug(level, fmt, args...) do {                        \
	if (level <= verbose)                                       \
		warnx(fmt, ##args);                                      \
} while (0)

#define logfatal(e, fmt, args...) do {                         \
	logmsg(LOG_DAEMON | LOG_ERR, errno, fmt, ##args);           \
	exit(e);                                                    \
} while (0)

#define MOUSED_OPTIONS "d:m:f:"

static struct rodentparam rodent = {
	.device = NULL,
	.modulename = NULL,
	.configfile = "/etc/moused.conf",
	.cfd = 0,
	.updatefunc = update,
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
		rodent.init(&rodent);
		if (rodent.probe(&rodent) == MODULE_PROBE_SUCCESS) {
			/* MAIN FUNCTION */
			rodent.run(&rodent);
		}
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
}

static void update(struct mouse_info *delta) {
	ioctl(rodent.cfd, CONS_MOUSECTL, delta);
}
