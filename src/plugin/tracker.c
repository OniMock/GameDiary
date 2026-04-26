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
#include "common/utils.h"
#include "plugin/detector.h"
#include "common/storage.h"
#include <psppower.h>
#include <pspsdk/systemctrl.h>

static SceUID tracker_thid = -1;
static SceUID cb_thid = -1;
static int running = 0;

static volatile u32 pending_seconds = 0;
static volatile u32 session_total_seconds = 0;
static SceOff current_session_offset = -1;
static volatile int is_suspended = 1; /* Start suspended until detector confirms */
static u32 current_game_uid = 0;

/*
 * Timestamp captured at the exact moment the current gaming session begins.
 * This value is IMMUTABLE for the lifetime of the session — it must NOT be
 * updated on each periodic flush, otherwise a session that crosses midnight
 * would be fully attributed to the following day instead of the day it started.
 */
static u32 session_start_ts = 0;

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
    is_suspended = 1;
  } else if (power_info & PSP_POWER_CB_RESUME_COMPLETE) {
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

  // Give memory stick and CFW time to settle
  sceKernelDelayThread(5 * 1000 * 1000);

  // Second pass to fetch ID directly from disc if parameters failed
  detector_init_late();

  cb_thid =
      sceKernelCreateThread("GameDiaryPwrCB", cb_thread, 0x30, 0x800, 0, 0);
  if (cb_thid >= 0) {
    sceKernelStartThread(cb_thid, 0, NULL);
  }

  // Initialize session (marks as a new launch)
  const GameMetadata *metadata = detector_get_metadata();
  
  utils_set_log_context(metadata->game_id);

  int st_res = storage_get_or_create_game(metadata, &current_game_uid);
  if (st_res < 0) {
    utils_log_error("tracker", "storage_get_or_create_game failed", st_res);
    current_game_uid = 0;
  }

  session_total_seconds = 0;
  current_session_offset = -1;
  /* Record the exact start time of this gaming session. This timestamp is
   * immutable and will be used for ALL writes of this session entry, ensuring
   * playtime is always attributed to the day the session began. */
  session_start_ts = utils_get_timestamp();
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
                                       0x30, 0x4000, 0, 0);
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
