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

/* sysmouse stuff */
#include <sys/mouse.h>
#include <sys/consio.h>

static int verbose = 0;

static void logmsg(int level, int errnum, const char *fmt, ...);
static int moused(int argc, char **argv);

#define debug(level, fmt, args...) do {                        \
	if (level <= verbose)                                       \
		warnx(fmt, ##args);                                      \
} while (0)

#define logfatal(e, fmt, args...) do {                         \
	logmsg(LOG_DAEMON | LOG_ERR, errno, fmt, ##args);           \
	exit(e);                                                    \
} while (0)

#define MOUSED_OPTIONS "p:m:f:"

static struct rodentparam {
	char *portname;
	char *modulename;
	char *configfile;
} rodent = {
	.portname = NULL,
	.modulename = NULL,
	.configfile = "/etc/moused.conf",
};

int main(int argc, char **argv) {
	int c;

	while (-1 != (c = getopt(argc, argv, MOUSED_OPTIONS))) {
		switch (c) {
			case 'p':
				rodent.portname = optarg; break;
			case 'm':
				rodent.modulename = optarg; break;
			case 'f':
				rodent.configfile = optarg; break;
		}
	}

	argc -= optind;
	argv += optind;

	moused(argc, argv);

	return 0;
}

int moused(int argc, char **argv) {
	int mousefd;
	struct mouse_info mouse;
	int extioctl = 0;
	int c;

	if (-1 == (mousefd = open("/dev/consolectl", O_RDWR, 0))) {
		logfatal(1, "Unable to open /dev/consolectl");
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
