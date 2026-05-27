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
  * @file tracker.c
  * @brief Game playtime tracking implementation.
  */

#include "plugin/tracker.h"
#include "common/common.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include "plugin/detector.h"
#include "common/storage.h"
#include <psppower.h>
#include <pspsdk/systemctrl.h>
#include "common/debug.h"

static SceUID tracker_thid = -1;
static int running = 0;

#define TRACKER_BASE_SETTLE_US       (15 * 1000 * 1000)
#define TRACKER_UMD_EXTRA_SETTLE_US  (30 * 1000 * 1000)
#define TRACKER_UMD_RETRY_DELAY_US   (15 * 1000 * 1000)
#define TRACKER_UMD_MAX_ATTEMPTS     16
#define UNKNOWN_GAME_ID              "UNKNOWN-00000"

static volatile u32 pending_seconds = 0;
static volatile u32 session_total_seconds = 0;
static SceOff current_session_offset = -1;
static u32 current_game_uid = 0;

/*
 * Timestamp captured at the exact moment the current gaming session begins.
 * This value is IMMUTABLE for the lifetime of the session — it must NOT be
 * updated on each periodic flush, otherwise a session that crosses midnight
 * would be fully attributed to the following day instead of the day it started.
 */
static u32 session_start_ts = 0;

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

// Power callback removed to prevent kernel panics with certain games.
// Using time-gap detection instead.

static void resolve_late_umd_metadata(void) {
  const GameMetadata *metadata = detector_get_metadata();
  if (!metadata_is_unresolved_psp(metadata)) {
    detector_init_late();
    return;
  }

  debug_log("tracker", "PSP metadata still UNKNOWN after base settle; waiting %u s before disc0 fallback.",
            (unsigned int)(TRACKER_UMD_EXTRA_SETTLE_US / 1000000));
  sceKernelDelayThread(TRACKER_UMD_EXTRA_SETTLE_US);

  for (u32 attempt = 1; running && attempt <= TRACKER_UMD_MAX_ATTEMPTS; attempt++) {
    debug_log("tracker", "Late UMD metadata attempt %u/%u",
              (unsigned int)attempt, (unsigned int)TRACKER_UMD_MAX_ATTEMPTS);

    if (detector_init_late()) {
      return;
    }

    metadata = detector_get_metadata();
    if (!metadata_is_unresolved_psp(metadata)) {
      return;
    }

    if (attempt < TRACKER_UMD_MAX_ATTEMPTS) {
      debug_log("tracker", "UMD metadata still UNKNOWN; retrying in %u s.",
                (unsigned int)(TRACKER_UMD_RETRY_DELAY_US / 1000000));
      sceKernelDelayThread(TRACKER_UMD_RETRY_DELAY_US);
    }
  }
}



static int tracker_thread_main(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  current_game_uid = 0;
  current_session_offset = -1;
  pending_seconds = 0;
  session_total_seconds = 0;
  session_start_ts = utils_get_timestamp();

  // Give memory stick and CFW time to settle.
  sceKernelDelayThread(TRACKER_BASE_SETTLE_US);

  // Second pass to fetch ID directly from disc if parameters failed.
  // GTA can keep the UMD driver busy during its intro, so unresolved UMDs
  // get a longer settle window and bounded retries before any storage entry
  // is created.
  resolve_late_umd_metadata();

  if (!running) {
    return 0;
  }

  const GameMetadata *metadata = detector_get_metadata();
  if (metadata_is_unresolved_psp(metadata)) {
    debug_log("tracker", "UMD metadata unresolved after %u attempts; tracking disabled to avoid unsafe disc0 I/O.",
              (unsigned int)TRACKER_UMD_MAX_ATTEMPTS);
    return 0;
  }



  // Initialize storage dynamically after the settle delay
  const char *prefix = utils_get_device_prefix();
  char base_dir[128];
  snprintf(base_dir, sizeof(base_dir), "%s%s", prefix, GDIARY_BASE_DIR);
  storage_init(base_dir);

  // Initialize session (marks as a new launch)
  utils_set_log_context(metadata->game_id);

  int st_res = storage_get_or_create_game(metadata, &current_game_uid);
  if (st_res < 0) {
    debug_log("tracker", "storage_get_or_create_game FAILED (res: %d)", st_res);
    utils_log_error("tracker", "storage_get_or_create_game failed", st_res);
    current_game_uid = 0;
  } else {
    debug_log("tracker", "Tracking Session Initialized (UID: %u, ID: %s)", current_game_uid, metadata->game_id);
  }

  session_total_seconds = 0;
  current_session_offset = -1;
  u32 now_ts = utils_get_timestamp();
  if (now_ts > session_start_ts) {
    session_total_seconds = now_ts - session_start_ts;
    debug_log("tracker", "Backfilled startup time before tracking init (%u s)",
              (unsigned int)session_total_seconds);
  }
  
  u32 last_tick_ts = utils_get_timestamp();

  while (running) {
    sceKernelDelayThread(1000 * 1000); // 1 sec

    now_ts = utils_get_timestamp();
    u32 diff = now_ts - last_tick_ts;

    // If more than 10 seconds passed since the last 1-second tick, 
    // the system was likely suspended in sleep mode!
    if (diff >= 10) {
      debug_log("tracker", "Large time gap detected (%u s). Assuming resume from sleep.", diff);
      
      if (current_game_uid > 0) {
        /* Flush any pending seconds accumulated before sleep,
         * attributing them to the OLD session (old day). */
        session_total_seconds += pending_seconds;
        pending_seconds = 0;
        if (session_total_seconds > 0) {
          storage_log_session(current_game_uid, session_total_seconds,
                                 session_start_ts, &current_session_offset);
          debug_log("tracker", "Pre-sleep flush (Total: %u s)", session_total_seconds);
        }

        /* Switch to local timestamp for day-boundary comparison so that
         * midnight on the PSP clock (00:00 local) triggers the split. */
        int local_offset = utils_get_timezone_offset_seconds();
        u32 start_day = (session_start_ts + local_offset) / 86400;
        u32 current_day = (now_ts + local_offset) / 86400;

        if (start_day != current_day) {
          debug_log("tracker", "Day changed during sleep (start_day=%u, current_day=%u). Starting new session.",
                    (unsigned int)start_day, (unsigned int)current_day);
          session_start_ts = now_ts;
          session_total_seconds = 0;
          current_session_offset = -1; /* Force storage to create a new record */
        }
      }
    } else {
      pending_seconds++;

      if (pending_seconds >= 60) {
        if (current_game_uid > 0) {
          session_total_seconds += pending_seconds;
          storage_log_session(current_game_uid, session_total_seconds,
                                 session_start_ts, &current_session_offset);
          debug_log("tracker", "Periodic Flush (Total: %u s)", session_total_seconds);
        }
        pending_seconds = 0;
      }
    }

    last_tick_ts = now_ts;
  }

  return 0;
}

void tracker_thread_start(void) {
  running = 1;
  tracker_thid = sceKernelCreateThread("GameDiaryTrk", tracker_thread_main,
                                       0x40, 0x4000, 0, 0);
  if (tracker_thid >= 0) {
    sceKernelStartThread(tracker_thid, 0, NULL);
  }
}

void tracker_thread_stop(void) {
  running = 0;
  if (pending_seconds > 0 && current_game_uid > 0) {
    session_total_seconds += pending_seconds;
    /* Consistent with all other flush points: use session_start_ts. */
    storage_log_session(current_game_uid, session_total_seconds,
                           session_start_ts, &current_session_offset);
    pending_seconds = 0;
  }
}
