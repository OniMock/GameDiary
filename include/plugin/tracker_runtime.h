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
 * @file tracker_runtime.h
 * @brief Tracker runtime implementation.
 */

#ifndef _PLUGIN_TRACKER_RUNTIME_H_
#define _PLUGIN_TRACKER_RUNTIME_H_

#include "common/models.h"
#include <psptypes.h>

typedef enum {
  TRACKER_RUNTIME_NOT_READY = 0,
  TRACKER_RUNTIME_BLOCKED,
  TRACKER_RUNTIME_ACTIVE
} TrackerRuntimeMode;

typedef struct {
  char base_dir[128];
  char game_id[16];
  u8   category;
  TrackerRuntimeMode mode;
  u8   tracking_enabled;
  u8   storage_ready;
  /** 1 = honor tracker.dat; 0 = always track (hotkey off / no plugin.dat). */
  u8   blocklist_active;
} TrackerRuntime;

/**
 * @param honor_tracker_blocklist  Non-zero only when plugin.dat enables hotkey.
 */
void tracker_runtime_init(TrackerRuntime *rt, const char *base_dir,
                          const GameMetadata *meta, int honor_tracker_blocklist);

/** @brief Re-read tracker.dat when blocklist_active (after toggle). */
void tracker_runtime_refresh_block(TrackerRuntime *rt);

/** @brief True when game_id is valid for persistence (all plugin categories). */
int tracker_runtime_metadata_ready(const GameMetadata *meta);

/** @brief True when playtime/icon/session logic may run. */
int tracker_runtime_is_tracking(const TrackerRuntime *rt);

TrackerRuntimeMode tracker_runtime_get_mode(const TrackerRuntime *rt);

/**
 * @brief Toggle block state when metadata is ready.
 * @return 1 if state changed, 0 if not ready or IO error, -1 if unchanged (duplicate).
 */
int tracker_runtime_toggle(TrackerRuntime *rt, const GameMetadata *meta);

#endif /* _PLUGIN_TRACKER_RUNTIME_H_ */
