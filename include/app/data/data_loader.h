/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_DATA_LOADER_H
#define GAMEDIARY_DATA_LOADER_H

#include "common/db_schema.h"

/**
 * @file data_loader.h
 * @brief Data loading and statistical structures for the app.
 */

typedef struct {
    GameEntry entry;
    u32 total_playtime;
    u32 session_count;
    u32 last_played_ts;
    u32 period_playtime;
} GameStats;

/**
 * @brief Loads all game and session data from disk.
 * @return 0 on success, negative on error.
 */
int data_load_all(void);

/**
 * @brief Calculates statistics for all games based on a time period.
 * @param start_ts Start UNIX timestamp.
 * @param end_ts End UNIX timestamp.
 */
void data_calculate_stats(u32 start_ts, u32 end_ts);

u32 data_get_game_count(void);
GameStats* data_get_games(void);

u32 data_get_session_count(void);
SessionEntry* data_get_sessions(void);

/**
 * @brief Frees loaded data resources.
 */
void data_free(void);

#endif // GAMEDIARY_DATA_LOADER_H
