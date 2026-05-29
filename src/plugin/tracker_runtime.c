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
 * @file tracker_runtime.c
 * @brief Per-session tracking enable/block state (all plugin categories).
 */

#include "plugin/tracker_runtime.h"
#include "plugin/tracker_db.h"
#include "common/db_schema.h"
#include <string.h>
#include <stdio.h>

#define UNKNOWN_GAME_ID "UNKNOWN-00000"

static int metadata_looks_like_disc_launch(const GameMetadata *metadata) {
  return metadata &&
         metadata->file_path[0] != '\0' &&
         strncmp(metadata->file_path, "disc0:/", 7) == 0;
}

static int metadata_is_unresolved_psp(const GameMetadata *metadata) {
  return metadata &&
         (metadata->category == CAT_PSP ||
          (metadata->category == CAT_UNKNOWN && metadata_looks_like_disc_launch(metadata))) &&
         strcmp(metadata->game_id, UNKNOWN_GAME_ID) == 0;
}

int tracker_runtime_metadata_ready(const GameMetadata *meta) {
  if (!meta || meta->game_id[0] == '\0') {
    return 0;
  }

  if (metadata_is_unresolved_psp(meta)) {
    return 0;
  }

  if (strcmp(meta->game_id, UNKNOWN_GAME_ID) == 0) {
    return 0;
  }

  return 1;
}

static void runtime_apply_block_state(TrackerRuntime *rt) {
  if (!rt || rt->game_id[0] == '\0') {
    return;
  }

  if (!rt->blocklist_active) {
    rt->mode = TRACKER_RUNTIME_ACTIVE;
    rt->tracking_enabled = 1;
    return;
  }

  if (tracker_db_contains(rt->game_id)) {
    rt->mode = TRACKER_RUNTIME_BLOCKED;
    rt->tracking_enabled = 0;
  } else {
    rt->mode = TRACKER_RUNTIME_ACTIVE;
    rt->tracking_enabled = 1;
  }
}

void tracker_runtime_init(TrackerRuntime *rt, const char *base_dir,
                          const GameMetadata *meta, int honor_tracker_blocklist) {
  if (!rt) {
    return;
  }

  memset(rt, 0, sizeof(*rt));
  rt->blocklist_active = honor_tracker_blocklist ? 1 : 0;

  if (base_dir) {
    snprintf(rt->base_dir, sizeof(rt->base_dir), "%s", base_dir);
  }

  char tracker_path[160];
  snprintf(tracker_path, sizeof(tracker_path), "%s/%s", rt->base_dir, TRACKER_DAT);
  tracker_db_set_path(tracker_path);

  if (meta) {
    snprintf(rt->game_id, sizeof(rt->game_id), "%s", meta->game_id);
    rt->category = meta->category;
  }

  if (!tracker_runtime_metadata_ready(meta)) {
    rt->mode = TRACKER_RUNTIME_NOT_READY;
    rt->tracking_enabled = 0;
    return;
  }

  runtime_apply_block_state(rt);
}

void tracker_runtime_refresh_block(TrackerRuntime *rt) {
  if (!rt || rt->game_id[0] == '\0') {
    return;
  }
  runtime_apply_block_state(rt);
}

int tracker_runtime_is_tracking(const TrackerRuntime *rt) {
  return rt && rt->tracking_enabled && rt->mode == TRACKER_RUNTIME_ACTIVE;
}

TrackerRuntimeMode tracker_runtime_get_mode(const TrackerRuntime *rt) {
  return rt ? rt->mode : TRACKER_RUNTIME_NOT_READY;
}

int tracker_runtime_toggle(TrackerRuntime *rt, const GameMetadata *meta) {
  if (!rt || !rt->blocklist_active || !meta || !tracker_runtime_metadata_ready(meta)) {
    return 0;
  }

  snprintf(rt->game_id, sizeof(rt->game_id), "%s", meta->game_id);
  rt->category = meta->category;

  if (rt->mode == TRACKER_RUNTIME_ACTIVE || rt->tracking_enabled) {
    if (tracker_db_add(rt->game_id) < 0) {
      return 0;
    }
    rt->mode = TRACKER_RUNTIME_BLOCKED;
    rt->tracking_enabled = 0;
    return 1;
  }

  if (tracker_db_remove(rt->game_id) < 0) {
    return 0;
  }
  rt->mode = TRACKER_RUNTIME_ACTIVE;
  rt->tracking_enabled = 1;
  return 1;
}
