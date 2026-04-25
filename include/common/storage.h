/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _COMMON_STORAGE_H_
#define _COMMON_STORAGE_H_

/* SceOff is defined in pspiofilemgr.h via pspkernel. */
#include "common/models.h"
#include <pspkernel.h>

/**
 * @brief Initializes the database directory structure and repairs any
 *        incomplete writes from a previous session.
 *
 * @param base_dir Full path to the GameDiary root directory, e.g.
 *                 "ms0:/PSP/COMMON/GameDiary". The db/ subdirectory
 *                 will be created inside. The plugin layer is responsible
 *                 for resolving this path (sctrlKernelMsIsEf-aware).
 */
void storage_init(const char *base_dir);

/**
 * @brief Looks up a game by game_id. Creates a new registry entry if absent.
 * @param meta   Metadata for the game (name, id, category, file_path).
 * @param uid    Output: assigned UID for this game.
 * @return 0 on success, negative on error.
 */
int storage_get_or_create_game(const GameMetadata *meta, u32 *uid);

/**
 * @brief Persists a session entry (duration, timestamp) to "SESSIONS_DAT".
 *        If *offset == -1 a new entry is appended; otherwise it overwrites
 *        the entry at the given offset (incremental update pattern).
 * @param game_uid  UID of the game being tracked.
 * @param duration  Total session duration in seconds.
 * @param timestamp UNIX timestamp of the update.
 * @param offset    In/out: byte offset of the session entry in "SESSIONS_DAT".
 * @return 0 on success, negative on error.
 */
int storage_log_session(u32 game_uid, u32 duration, u32 timestamp, SceOff *offset);

#endif /* _COMMON_STORAGE_H_ */
