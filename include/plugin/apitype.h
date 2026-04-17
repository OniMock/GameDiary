/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_APITYPE_H_
#define _PLUGIN_APITYPE_H_

/* Only needs CAT_* constants from models, not the full common header. */
#include "common/models.h"

/**
 * @brief Detects the game category (PSP, PS1, Homebrew, VSH) from the apitype.
 * @param apitype Value returned by sceKernelInitApitype().
 * @return One of the CAT_* constants defined in models.h.
 */
int apitype_detect_category(int apitype);

#endif /* _PLUGIN_APITYPE_H_ */
