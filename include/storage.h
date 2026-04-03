#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "models.h"

// Initialize database directory and check integrity
void storage_init(void);

// Lookup game UID by game_id. Create if not found.
// Returns 0 on success, < 0 on error.
int storage_get_or_create_game(const GameMetadata *meta, u32 *uid);

// Append a new session record to sessions.dat
// Returns 0 on success, < 0 on error.
int storage_append_session(u32 game_uid, u32 duration, u32 timestamp);

#endif
