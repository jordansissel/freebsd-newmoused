/* 
 * Demonstrative newmoused module
 *
 * Reads text-based commands from standard input tellint
 * the mouse how to move, etc.
 */

#include <sys/types.h>

#include <sys/mouse.h>
#include <sys/consio.h>

#include "../../moused.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>

static const int BUFLEN = 512;

static void command(rodent_t *rodent, char *buf, char *eol);

/* 
 * Normally we'd be poking rodent.device and such to figure out
 * if it's the kind of mouse we support. However, since this 
 * module doesn't actually access mouse hardware, we will
 * always return SUCCESS.
 *
 * XXX: Should we return FAIL if there is a device specified?
 * I think so..
 */
MOUSED_PROBE_FUNC {
	return MODULE_PROBE_SUCCESS;
}

MOUSED_INIT_FUNC {
	int c;

	while (-1 != (c = getopt(argc, argv, "t:"))) {
		switch (c) {
			case 't':
				printf("'t' option: %s\n", optarg); break;
			default:
				printf("Foo: %c\n", optopt);
		} 
	}
	
}

MOUSED_RUN_FUNC {
	char buf[BUFLEN];
	int len = 0;
	memset(buf, 0, BUFLEN);

	while (!feof(stdin)) {
		int bytes;
		char *eol;

		/* Read data from stdin */
		if (len < BUFLEN) {
			bytes = fread(buf + len, 1, 1, stdin);
			len += bytes;
		} else {
			/* we've hit buffer length, line is definately too long, reset */
			warnx("Input buffer full, clearing... (Line too long?)");
			len = 0;
		}

		/* Process data */
		while (NULL != (eol = strchr(buf, '\n'))) {
			command(rodent, buf, eol);
			strlcpy(buf, (eol + 1), BUFLEN);
			len -= (eol - buf) + 1;
		}
	}
}

static void command(rodent_t *rodent, char *buf, char *eol) {
	char *string;
	char *tok;
	int len = strlen(buf);
	mouse_info_t delta;
	
	/* Null the EOL so we can do string functions on it */
	*eol = '\0';

	memset(&delta, 0, sizeof(mouse_info_t));

	string = malloc(len + 1);
	strlcpy(string, buf, len + 1);

	tok = strsep(&string, " ");

	/* XXX: MOUSE_MOTION_EVENT and MOUSE_ACTION produce the same results?! */
	delta.operation = MOUSE_MOTION_EVENT;
	//delta.operation = MOUSE_ACTION;
	if (strcmp("movex", tok) == 0) {
		tok = strsep(&string, " ");
		delta.u.data.x = atoi(tok);
	} else if (strcmp("movey", tok) == 0) {
		tok = strsep(&string, " ");
		delta.u.data.y = atoi(tok);
	} else if (strcmp("click", tok) == 0) {
		while (NULL != (tok = strsep(&string, " "))) {
			int button = atoi(tok);
			if (button > 0 && button < 32)
				delta.u.data.buttons |= (1 << button - 1);
			else
				warnx("Invalid button: %d", button);
		}
		printf("Buttons: %08x\n", delta.u.data.buttons);
		rodent->update(&delta);
		delta.u.data.buttons = 0;
		printf("Buttons: %08x\n", delta.u.data.buttons);
	} else {
		printf("Unknown command: %s\n", tok);
	}

	rodent->update(&delta);
}
