/* 
 * new moused module for ums(4) mice
 *
 */

#include <sys/types.h>
#include <sys/mouse.h>
#include <sys/consio.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <err.h>
#include <errno.h>

#include "../../moused.h"

static void (*logmsg)(int, int, const char *, ...) = NULL;

static void activity(rodent_t *rodent, char *packet);
static void printbits(char packet);
static int tryproto(rodent_t *rodent, int proto);
static char *getmouseproto(int proto);

static int mouseproto = MOUSE_PROTO_SYSMOUSE;

static struct mouse_protomap {
	int proto;
	char *name;
} protomap[] = {
	{ .proto = MOUSE_PROTO_SYSMOUSE, .name = "sysmouse" },
	{ .proto = MOUSE_PROTO_MSC, .name = "mousesystems" },
};

MOUSED_INIT_FUNC {
	int c;
	char *type = "sysmouse";
	logmsg = rodent->logmsg;

	while (-1 != (c = getopt(argc, argv, "t:"))) {
		switch (c) {
			case 't':
				type = optarg; break;
		}
	}

	if (strcmp(type, "sysmouse") == 0) {
		mouseproto = MOUSE_PROTO_SYSMOUSE;
	} else if ((strcmp(type, "mousesystems") == 0)
				  || (strcmp(type, "msc") == 0)) {
		mouseproto = MOUSE_PROTO_MSC;
	} else {
		warnx("Unknown protocol '%s' - defaulting to sysmouse", type);
	}
}

MOUSED_PROBE_FUNC {
	int level = 0;
	rodent->mfd = open(rodent->device, O_RDWR);

	if (-1 == rodent->mfd)
		logfatal(1, "Unable to open %s", rodent->device);

	if (tryproto(rodent, mouseproto) == 0) { 
		/* Try the other protocol */
	mouseproto = (mouseproto == MOUSE_PROTO_SYSMOUSE ? 
						  MOUSE_PROTO_MSC : MOUSE_PROTO_SYSMOUSE);
		if (tryproto(rodent, mouseproto) == 0)
			return MODULE_PROBE_FAIL;
	}

	return MODULE_PROBE_SUCCESS;
}

MOUSED_RUN_FUNC {
	char *packet;
	fd_set fds;

	ioctl(rodent->mfd, MOUSE_GETHWINFO, &(rodent->hw));
	printf("NumButtons: %d\n", rodent->hw.buttons);

	packet = malloc(rodent->mode.packetsize);

	FD_ZERO(&fds);
	FD_SET(rodent->mfd, &fds);

	for (;;) {
		int tries = 0;
		int bytes, x, c;

		c = select(FD_SETSIZE, &fds, NULL, NULL, NULL); /* wait forever */
		if (c < 0) {
			logfatal(1, "Error with select call");
		} else if (c > 0) {
			bytes = read(rodent->mfd, packet, rodent->mode.packetsize); 
			if (bytes < 0)
				logfatal(1, "Error reading from mousey");
		
			if (bytes == rodent->mode.packetsize)
				activity(rodent, packet);
		}
	}
}

static void activity(rodent_t *rodent, char *packet) {
	mouse_info_t delta;
	int x;

	delta.operation = MOUSE_ACTION;
	delta.u.data.buttons = 0;
	delta.u.data.x = delta.u.data.y = delta.u.data.z = 0;

	if (0)
	{
		for (x = 0; x < 8; x++)
			printf("%02x ", (*(packet + x) & 0xff));
		printf("\n");
		printbits(*packet);
	}

	/* sysmouse(4) details what is in the 8-byte packets */

	delta.u.data.buttons |= NBIT(*packet, 3) 
		                     | NBIT(*packet, 2) << 1
									| NBIT(*packet, 1) << 2;

	/* Bytes 2 and 4 are horizontal */
	delta.u.data.x = (*(packet + 1) + *(packet + 3)); 

	/* Bytes 3 and 5 are vertical */
	delta.u.data.y = 0 - (*(packet + 2) + *(packet + 4)); 

	/* sysmouse adds z-axis changes */
	if (MOUSE_PROTO_SYSMOUSE == mouseproto)
		delta.u.data.z = (*(packet + 5) + *(packet + 6));

	//printf("%d\n", delta.u.data.z);
	//printf("(%d,%d) ", delta.u.data.x, delta.u.data.y);
	//printbits(delta.u.data.buttons);

	rodent->update(&delta);
}

static void printbits(char byte) {
	int x;
	for (x = 1; x <= 8; x++) {
		printf("%d", BIT(byte, x));
	}
	printf("\n");
}

static int tryproto(rodent_t *rodent, int proto) {
	int level;
	warnx("Trying %s protocol", getmouseproto(proto));

	for (level = 0; level < 3; level++) {
		/* Set the driver operation level */
		ioctl(rodent->mfd, MOUSE_SETLEVEL, &level);
		ioctl(rodent->mfd, MOUSE_GETMODE, &(rodent->mode));
		if (mouseproto == rodent->mode.protocol)
			return 1;
	}

	return 0;
}

static char *getmouseproto(int proto) {
	int x;
	for (x = 0; x < (sizeof(protomap) / sizeof(struct mouse_protomap)); x++) {
		if (protomap[x].proto == proto)
			return protomap[x].name;
	}
}
