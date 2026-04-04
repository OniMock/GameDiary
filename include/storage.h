#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "models.h"

// Initialize database directory and check integrity
void storage_init(void);

// Lookup game UID by game_id. Create if not found.
// Returns 0 on success, < 0 on error.
int storage_get_or_create_game(const GameMetadata *meta, u32 *uid);

/**
 * Logs a session entry.
 * If *offset == -1, appends a new entry and saves its offset.
 * If *offset >= 0, overwrites the entry at that offset.
 */
int storage_log_session(u32 game_uid, u32 duration, u32 timestamp, SceOff *offset);

#endif
