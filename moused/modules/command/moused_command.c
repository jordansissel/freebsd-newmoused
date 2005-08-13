#include <sys/types.h>

#include <sys/mouse.h>
#include <sys/consio.h>

#include "../../moused.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

MOUSED_INIT_FUNC {
	update = rodent->updatefunc;
}

MOUSED_PROBE_FUNC {
	return MODULE_PROBE_SUCCESS;
}

MOUSED_RUN_FUNC {
	struct mouse_info m;
	memset(&m, 0, sizeof(struct mouse_info));

	for (;;) {
		m.operation = MOUSE_MOTION_EVENT;
		m.u.data.x = -3;
		update(&m);
		sleep(1);
	}

}
