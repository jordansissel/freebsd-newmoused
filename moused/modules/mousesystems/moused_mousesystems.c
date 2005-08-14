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

MOUSED_PROBE_FUNC {
	rodent->mfd = open(rodent->device, O_RDWR);

	if (-1 == rodent->mfd)
		logfatal(1, "Unable to open %s", rodent->device);

	return MODULE_PROBE_SUCCESS;
}
