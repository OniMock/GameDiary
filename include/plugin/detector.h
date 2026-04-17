/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_DETECTOR_H_
#define _PLUGIN_DETECTOR_H_

#include "common/models.h"

/**
 * @brief Immediately fetches game metadata at boot time (early, via sctrlGetInitPARAM).
 * Should be called from module_start before any thread is spawned.
 */
void detector_init(void);

/**
 * @brief Fetches game metadata again after the system has settled (via UMD).
 * Called from the tracker thread after a startup delay.
 */
void detector_init_late(void);

/**
 * @brief Returns a const pointer to the cached game metadata.
 */
const GameMetadata *detector_get_metadata(void);

#endif /* _PLUGIN_DETECTOR_H_ */
