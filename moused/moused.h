
#ifndef MOUSED_H
#define MOUSED_H

struct rodentparam;
typedef struct rodentparam rodent_t;


struct rodentparam {
	char *device;
	char *modulename;
	char *configfile;
	int cfd;

	/* Modules will call this to pass deltas to sysmouse(4) */
	int (*update)(struct mouse_info *);

	/* Module callbacks (called by moused) */
	int (*init)(rodent_t *, int, char **);
	int (*probe)(rodent_t *);
	int (*run)(rodent_t *);
};

#define MOUSED_INIT_FUNC  int init(rodent_t *rodent, int argc, char **argv)
#define MOUSED_PROBE_FUNC int probe(rodent_t *rodent)
#define MOUSED_RUN_FUNC   void run(rodent_t *rodent)

/* Probe function return values */
#define MODULE_PROBE_FAIL    0
#define MODULE_PROBE_SUCCESS 1

#endif
