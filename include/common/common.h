#ifndef _COMMON_COMMON_H_
#define _COMMON_COMMON_H_

/* Basic PSP SDK headers needed by both common and plugin layers.
 * This header deliberately excludes kernel-only includes such as
 * systemctrl.h so that it can be safely consumed by user-mode code. */

#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <stdio.h>
#include <string.h>

#include "common/models.h"

#endif /* _COMMON_COMMON_H_ */
