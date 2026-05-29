/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file tracker_db.h
 * @brief Tracker database implementation.
 */

#ifndef _PLUGIN_TRACKER_DB_H_
#define _PLUGIN_TRACKER_DB_H_

#include <psptypes.h>

#define TRACKER_GAME_ID_LEN  16
#define TRACKER_ENTRY_SIZE   16
#define TRACKER_DB_VERSION   1
#define TRACKER_MAGIC        0x4B415254 /* 'TRAK' */

#pragma pack(push, 1)
typedef struct {
  char game_id[TRACKER_GAME_ID_LEN];
} TrackerEntry;

typedef struct {
  u32 magic;
  u32 version;
  u32 entry_count;
  u32 reserved;
} TrackerHeader;
#pragma pack(pop)

/**
 * @brief Full path to tracker.dat (prefix + GDIARY_BASE_DIR + "/tracker.dat").
 */
void tracker_db_set_path(const char *tracker_dat_path);

/** @brief Incremental scan; returns 1 if game_id is blocked. */
int tracker_db_contains(const char *game_id);

/** @brief Appends game_id if absent; creates file/header when needed. */
int tracker_db_add(const char *game_id);

/** @brief Removes game_id; rewrites file cooperatively. */
int tracker_db_remove(const char *game_id);

#endif /* _PLUGIN_TRACKER_DB_H_ */
