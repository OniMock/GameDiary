#include "tracker.h"
#include "common.h"
#include "detector.h"
#include "storage.h"

static SceUID tracker_thid = -1;
static SceUID cb_thid = -1;
static int running = 0;

static volatile u32 pending_seconds = 0;
static volatile int is_suspended = 0;

// UNIX timestamp via RTC
u32 get_current_timestamp(void) {
  u64 tick;
  sceRtcGetCurrentTick(&tick);
  // tick is microseconds since 0001-01-01.
  // UNIX epoch is 1970-01-01. Difference is roughly 62135596800 seconds.
  u32 ts = (u32)((tick / 1000000ULL) - 62135596800ULL);
  return ts;
}

static int power_callback(int unknown, int power_info, void *arg) {
  (void)unknown;
  (void)arg;

  if (power_info & PSP_POWER_CB_POWER_SWITCH ||
      power_info & PSP_POWER_CB_SUSPENDING) {
    if (pending_seconds > 0) {
      storage_update_session(g_game_id, g_game_name, g_category,
                             pending_seconds, 0);
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
  storage_update_session(g_game_id, g_game_name, g_category, 0, 1);

  while (running) {
    sceKernelDelayThread(1000 * 1000); // 1 sec

    if (!is_suspended) {
      pending_seconds++;

      if (pending_seconds >= 60) {
        storage_update_session(g_game_id, g_game_name, g_category,
                               pending_seconds, 0);
        pending_seconds = 0;
      }
    }
  }

  return 0;
}

void tracker_thread_start(void) {
  running = 1;
  tracker_thid = sceKernelCreateThread("GameDiaryTrk", tracker_thread_main,
                                       0x30, 0x1000, 0, 0);
  if (tracker_thid >= 0) {
    sceKernelStartThread(tracker_thid, 0, NULL);
  }
}

void tracker_thread_stop(void) {
  running = 0;
  if (pending_seconds > 0) {
    storage_update_session(g_game_id, g_game_name, g_category, pending_seconds,
                           0);
    pending_seconds = 0;
  }
}
