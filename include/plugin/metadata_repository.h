/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_METADATA_REPOSITORY_H_
#define _PLUGIN_METADATA_REPOSITORY_H_

#include "common/models.h"

/**
 * @brief Fills GameMetadata from the PSP system (sctrlGetInitPARAM, kubridge).
 *        Handles PSP, PS1 (PBP SFO), and Homebrew ID hashing.
 * @param metadata Pointer to the struct to fill.
 * @return 1 on success, 0 otherwise.
 */
int metadata_fetch(GameMetadata *metadata);

/**
 * @brief Late fetch from disc0:/UMD_DATA.BIN (after UMD is ready).
 *        Used as a fallback when early fetch returned UNKNOWN.
 * @param metadata Pointer to the struct to fill.
 * @return 1 on success, 0 otherwise.
 */
int metadata_fetch_from_umd(GameMetadata *metadata);

#endif /* _PLUGIN_METADATA_REPOSITORY_H_ */
