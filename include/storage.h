#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "common.h"

// Initialize database directory
void storage_init(void);

// Atomically save or append the session playtime to the database
void storage_update_session(const char *game_id, const char *game_name,
                            const char *apitype_str, u8 category,
                            u32 session_time, int is_new_session);

#endif
