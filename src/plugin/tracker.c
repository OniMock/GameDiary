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
static volatile int is_suspended = 1; // Start suspended until detector confirms
static u32 current_game_uid = 0;

static int power_callback(int unknown, int power_info, void *arg) {
  (void)unknown;
  (void)arg;

  if (power_info & PSP_POWER_CB_POWER_SWITCH ||
      power_info & PSP_POWER_CB_SUSPENDING) {
    if (pending_seconds > 0 && current_game_uid > 0) {
      session_total_seconds += pending_seconds;
      storage_log_session(current_game_uid, session_total_seconds,
                             utils_get_timestamp(), &current_session_offset);
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
  if (storage_get_or_create_game(metadata, &current_game_uid) < 0) {
    current_game_uid = 0;
  }

  session_total_seconds = 0;
  current_session_offset = -1;
  is_suspended = 0; // Everything ready

  while (running) {
    sceKernelDelayThread(1000 * 1000); // 1 sec

    if (!is_suspended) {
      pending_seconds++;

      if (pending_seconds >= 60) {
        if (current_game_uid > 0) {
          session_total_seconds += pending_seconds;
          storage_log_session(current_game_uid, session_total_seconds,
                                 utils_get_timestamp(), &current_session_offset);
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
    storage_log_session(current_game_uid, session_total_seconds,
                           utils_get_timestamp(), &current_session_offset);
    pending_seconds = 0;
  }
}
