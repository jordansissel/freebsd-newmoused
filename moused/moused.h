
#ifndef MOUSED_H
#define MOUSED_H

#include <syslog.h>

struct rodentparam;
typedef struct rodentparam rodent_t;

struct rodentparam {
	char *device;
	char *modulename;
	char *configfile;
	int cfd;            /* /dev/consolectl fd */
	int mfd;            /* mouse device fd */

	mousemode_t mode;
	mousehw_t hw;
	mouse_info_t state;

	char *packet;    /* Used repeatedly by a plugin, keep it here */

	/* Modules will call this to pass deltas to sysmouse(4) */
	int (*update)(struct mouse_info *);
	void (*logmsg)(int, int, const char *, ...);

	/* Module callbacks (called by moused) */
	int (*init)(rodent_t *, int, char **);
	int (*probe)(rodent_t *);
	void (*run)(rodent_t *);                /* for standalone mode */

	void (*handler)(rodent_t *);            /* for packet-handler mode */
	void (*handler_init)(rodent_t *);       /* for packet-handler init*/

	void (*timeout)(rodent_t *);            /* select(2) timeouts call this 
														  * when in handler mode */
};

#define MOUSED_INIT_FUNC  int init(rodent_t *rodent, int argc, char **argv)
#define MOUSED_PROBE_FUNC int probe(rodent_t *rodent)
#define MOUSED_HANDLER_FUNC void handler(rodent_t *rodent)
#define MOUSED_HANDLER_INIT_FUNC void handler_init(rodent_t *rodent)

/*
 * *** XXX:DEPRECATED ***
 * Should this (RUN) method even be allowed? Is it useful to have the plugin 
 * handle things like select(2) calls and such?
 */
#define MOUSED_RUN_FUNC   void run(rodent_t *rodent)

/* Probe function return values */
#define MODULE_PROBE_FAIL    0
#define MODULE_PROBE_SUCCESS 1

/* Bit math, helpful for modules */
#define BIT(num, bit) (((num) & (1 << (bit - 1))) ? 1 : 0)
#define NBIT(num, bit) ((BIT(num,bit)) ^ 1)

#define logfatal(e, fmt, args...) do {                         \
	logmsg(LOG_DAEMON | LOG_ERR, errno, fmt, ##args);           \
	exit(e);                                                    \
} while (0)

#endif
