/* 
 * new moused module for ums(4) mice
 *
 */

#include <sys/types.h>
#include <sys/mouse.h>
#include <sys/consio.h>
#include <fcntl.h>

#include <stdio.h>
#include <err.h>
#include <errno.h>

#include "../../moused.h"

static void (*logmsg)(int, int, const char *, ...) = NULL;

static void activity(rodent_t *rodent, char *packet);
static void printbits(char packet);

MOUSED_INIT_FUNC {
	logmsg = rodent->logmsg;
}

MOUSED_PROBE_FUNC {
	int level = 0;
	rodent->mfd = open(rodent->device, O_RDWR);

	if (-1 == rodent->mfd)
		logfatal(1, "Unable to open %s", rodent->device);

	for (level = 0; level < 3; level++) {
		/* Set the driver operation level */
		ioctl(rodent->mfd, MOUSE_SETLEVEL, &level);
		ioctl(rodent->mfd, MOUSE_GETMODE, &(rodent->mode));
		if (MOUSE_PROTO_MSC == rodent->mode.protocol)
			break;
	}
	if (MOUSE_PROTO_MSC != rodent->mode.protocol)
		return MODULE_PROBE_FAIL;

	return MODULE_PROBE_SUCCESS;
}

MOUSED_RUN_FUNC {
	char *packet;

	ioctl(rodent->mfd, MOUSE_GETHWINFO, &(rodent->hw));
	printf("NumButtons: %d\n", rodent->hw.buttons);

	packet = malloc(rodent->mode.packetsize);

	for (;;) {
		int bytes, x;
		bytes = read(rodent->mfd, packet, rodent->mode.packetsize);
		if (bytes < 0)
			logfatal(1, "Error reading from mousey");
		
		if (bytes == rodent->mode.packetsize)
			activity(rodent, packet);
	}
}

static void activity(rodent_t *rodent, char *packet) {
	mouse_info_t delta;
	int x;

	delta.operation = MOUSE_ACTION;
	delta.u.data.buttons = 0;
	delta.u.data.x = delta.u.data.y = delta.u.data.z = 0;

	/*
	for (x = 0; x < 8; x++)
		printf("%02x ", (*(packet + x) & 0xff));
	printf("\n");
	*/
	//printbits(*packet);

	/* mouse(4) details what is in the 5-byte packets */

	delta.u.data.buttons |= NBIT(*packet, 3) 
		                     | NBIT(*packet, 2) << 1
									| NBIT(*packet, 1) << 2;

	/* Bytes 2 and 4 are horizontal */
	delta.u.data.x = (*(packet + 1) + *(packet + 3)); 

	/* Bytes 3 and 5 are vertical */
	delta.u.data.y = 0 - (*(packet + 2) + *(packet + 4)); 

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
