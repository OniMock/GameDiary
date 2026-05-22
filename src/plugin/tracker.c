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
static SceUID cb_thid = -1;
static int running = 0;

#define TRACKER_BASE_SETTLE_US       (15 * 1000 * 1000)
#define TRACKER_UMD_EXTRA_SETTLE_US  (30 * 1000 * 1000)
#define TRACKER_UMD_RETRY_DELAY_US   (15 * 1000 * 1000)
#define TRACKER_UMD_MAX_ATTEMPTS     16
#define UNKNOWN_GAME_ID              "UNKNOWN-00000"

static volatile u32 pending_seconds = 0;
static volatile u32 session_total_seconds = 0;
static SceOff current_session_offset = -1;
static volatile int is_suspended = 1; /* Start suspended until detector confirms */
static u32 current_game_uid = 0;
static int cb_thread(SceSize args, void *argp);

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

static void start_power_callback_thread(void) {
  cb_thid =
      sceKernelCreateThread("GameDiaryPwrCB", cb_thread, 0x30, 0x800, 0, 0);
  if (cb_thid >= 0) {
    sceKernelStartThread(cb_thid, 0, NULL);
  }
}

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

static int power_callback(int unknown, int power_info, void *arg) {
  (void)unknown;
  (void)arg;

  if (power_info & PSP_POWER_CB_POWER_SWITCH ||
      power_info & PSP_POWER_CB_SUSPENDING) {
    if (pending_seconds > 0 && current_game_uid > 0) {
      session_total_seconds += pending_seconds;
      /* Use session_start_ts, NOT the current time. The session must be
       * attributed to the day it started, even if we cross midnight. */
      storage_log_session(current_game_uid, session_total_seconds,
                             session_start_ts, &current_session_offset);
      pending_seconds = 0;
    }
    debug_log("tracker", "Power Callback: Suspending (Total: %u s)", session_total_seconds);
    is_suspended = 1;
  } else if (power_info & PSP_POWER_CB_RESUME_COMPLETE) {
    debug_log("tracker", "Power Callback: Resumed");
    is_suspended = 0;
  }

  return 0;
}

static int cb_thread(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  int cbid = sceKernelCreateCallback("PwrCB", power_callback, NULL);
  if (cbid >= 0) {
    scePowerRegisterCallback(0, cbid);
    sceKernelSleepThreadCB();
  }
  return 0;
}

static int tracker_thread_main(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  current_game_uid = 0;
  current_session_offset = -1;
  pending_seconds = 0;
  session_total_seconds = 0;
  is_suspended = 1;
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

  start_power_callback_thread();

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
  is_suspended = 0; /* Everything ready */

  while (running) {
    sceKernelDelayThread(1000 * 1000); // 1 sec

    if (!is_suspended) {
      pending_seconds++;

      if (pending_seconds >= 60) {
        if (current_game_uid > 0) {
          session_total_seconds += pending_seconds;
          /* Use session_start_ts (immutable) so the session always belongs
           * to the day it started, regardless of when this flush happens. */
          storage_log_session(current_game_uid, session_total_seconds,
                                 session_start_ts, &current_session_offset);
          debug_log("tracker", "Periodic Flush (Total: %u s)", session_total_seconds);
        }
        pending_seconds = 0;
      }
    }
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
