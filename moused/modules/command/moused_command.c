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

/* 
 * Normally we'd be poking rodent.device and such to figure out
 * if it's the kind of mouse we support. However, since this 
 * kA
 */
MOUSED_PROBE_FUNC {
	return MODULE_PROBE_SUCCESS;
}

MOUSED_RUN_FUNC {
	struct mouse_info m;
	memset(&m, 0, sizeof(struct mouse_info));

	for (;;) {
		m.operation = MOUSE_MOTION_EVENT;
		m.u.data.x = -3;
		rodent->update(&m);
		sleep(1);
	}
}
