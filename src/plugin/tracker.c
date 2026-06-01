/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 * -------------------------------------------------------------
 */

#include "plugin/tracker.h"
#include "common/common.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include "plugin/detector.h"
#include "common/storage.h"
#include "plugin/plugin_config.h"
#include "plugin/tracker_runtime.h"
#include "plugin/input_combo.h"
#include "plugin/overlay_notification.h"
#include <pspdisplay.h>
#include <psppower.h>
#include <pspsdk/systemctrl.h>
#include "common/debug.h"

static SceUID tracker_thid = -1;
static int running = 0;

#define TRACKER_BASE_SETTLE_US       (15 * 1000 * 1000)
#define TRACKER_UMD_EXTRA_SETTLE_US  (30 * 1000 * 1000)
#define TRACKER_UMD_RETRY_DELAY_US   (15 * 1000 * 1000)
#define TRACKER_UMD_MAX_ATTEMPTS     16
#define TRACKER_INPUT_SLICE_US       (100 * 1000)
#define TRACKER_INPUT_SLICES_PER_SEC 10
#define UNKNOWN_GAME_ID              "UNKNOWN-00000"

static volatile u32 pending_seconds = 0;
static volatile u32 session_total_seconds = 0;
static SceOff current_session_offset = -1;
static u32 current_game_uid = 0;
static u32 session_start_ts = 0;
static u32 s_last_tick_ts = 0;

static TrackerRuntime g_runtime;

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

static void session_flush_pending(void) {
  if (current_game_uid == 0) {
    pending_seconds = 0;
    return;
  }

  session_total_seconds += pending_seconds;
  pending_seconds = 0;

  if (session_total_seconds > 0) {
    storage_log_session(current_game_uid, session_total_seconds,
                        session_start_ts, &current_session_offset);
  }
}

/** Local calendar day index (midnight boundary uses PSP timezone). */
static u32 session_calendar_day(u32 ts) {
  int local_offset = utils_get_timezone_offset_seconds();
  return (ts + (u32)local_offset) / 86400U;
}

static int session_same_calendar_day(u32 ts_a, u32 ts_b) {
  return session_calendar_day(ts_a) == session_calendar_day(ts_b);
}

/** Flush current session and open a new DB entry dated from now_ts. */
static void session_start_new_day(u32 now_ts) {
  if (current_game_uid == 0) {
    return;
  }

  u32 prev_day = session_calendar_day(session_start_ts);
  u32 new_day = session_calendar_day(now_ts);

  session_flush_pending();
  debug_log("tracker", "Day changed (day %u -> %u). Starting new session.",
            (unsigned int)prev_day, (unsigned int)new_day);
  session_start_ts = now_ts;
  session_total_seconds = 0;
  current_session_offset = -1;
  pending_seconds = 0;
}

/** Pause tracking; keep session state for same-day resume. */
static void session_pause_tracking(void) {
  session_flush_pending();
  pending_seconds = 0;
}

static int session_begin_tracking(const GameMetadata *metadata, int new_session) {
  if (!metadata || !g_runtime.storage_ready) {
    return -1;
  }

  utils_set_log_context(metadata->game_id);

  if (current_game_uid == 0) {
    int st_res = storage_get_or_create_game(metadata, &current_game_uid);
    if (st_res < 0) {
      debug_log("tracker", "storage_get_or_create_game FAILED (res: %d)", st_res);
      utils_log_error("tracker", "storage_get_or_create_game failed", st_res);
      current_game_uid = 0;
      return st_res;
    }
    new_session = 1;
  }

  if (new_session) {
    session_flush_pending();
    session_start_ts = utils_get_timestamp();
    session_total_seconds = 0;
    current_session_offset = -1;
    debug_log("tracker", "New session (UID: %u, ID: %s)",
              current_game_uid, metadata->game_id);
  } else {
    debug_log("tracker", "Session resumed (UID: %u, ID: %s, %u s so far)",
              current_game_uid, metadata->game_id,
              (unsigned int)session_total_seconds);
  }

  pending_seconds = 0;
  return 0;
}

static void storage_ensure_initialized(const char *base_dir) {
  if (g_runtime.storage_ready) {
    return;
  }
  storage_init(base_dir);
  g_runtime.storage_ready = 1;
}

static void handle_combo_event(InputComboEvent ev, const GameMetadata *metadata) {
  if (ev == INPUT_COMBO_NOT_READY) {
    debug_log("tracker", "Combo ignored: metadata not ready yet");
    overlay_notification_show("GameDiary - Tracker not ready",
                              "GameDiary - Please wait...",
                              OVERLAY_NOTIFY_NEUTRAL);
    return;
  }

  if (ev != INPUT_COMBO_TOGGLE) {
    return;
  }

  if (!metadata) {
    debug_log("tracker", "Combo toggle ignored: metadata is NULL");
    return;
  }

  int was_tracking = tracker_runtime_is_tracking(&g_runtime);

  if (!tracker_runtime_toggle(&g_runtime, metadata)) {
    debug_log("tracker", "Combo toggle failed for %s (cat %u)",
              metadata->game_id, (unsigned int)metadata->category);
    return;
  }

  if (was_tracking) {
    session_pause_tracking();
    overlay_notification_show("GameDiary - Tracker: OFF", NULL,
                              OVERLAY_NOTIFY_TRACKER_OFF);
    debug_log("tracker", "User disabled tracking for %s (cat %u)",
              metadata->game_id, (unsigned int)metadata->category);
  } else {
    const char *prefix = utils_get_device_prefix();
    char base_dir[128];
    u32 now_ts = utils_get_timestamp();
    int new_session;

    snprintf(base_dir, sizeof(base_dir), "%s%s", prefix, GDIARY_BASE_DIR);
    storage_ensure_initialized(base_dir);

    new_session = (current_game_uid == 0) ||
                  !session_same_calendar_day(session_start_ts, now_ts);

    debug_log("tracker", "Combo ON request for %s (cat %u): new_session=%d "
              "(uid=%u, session_day=%u, now_day=%u)",
              metadata->game_id, (unsigned int)metadata->category, new_session,
              (unsigned int)current_game_uid,
              (unsigned int)session_calendar_day(session_start_ts),
              (unsigned int)session_calendar_day(now_ts));

    if (session_begin_tracking(metadata, new_session) == 0) {
      s_last_tick_ts = now_ts;
      overlay_notification_show("GameDiary - Tracker: ON", NULL,
                                OVERLAY_NOTIFY_TRACKER_ON);
      debug_log("tracker", "User enabled tracking for %s (cat %u)",
                metadata->game_id, (unsigned int)metadata->category);
    } else {
      debug_log("tracker", "Combo ON failed: session_begin_tracking for %s (cat %u)",
                metadata->game_id, (unsigned int)metadata->category);
    }
  }
}

static void playtime_tick(u32 now_ts) {
  if (!tracker_runtime_is_tracking(&g_runtime) || current_game_uid == 0) {
    return;
  }

  u32 diff = now_ts - s_last_tick_ts;

  /* Midnight while tracker ON: always start a new session for the new day. */
  if (!session_same_calendar_day(session_start_ts, now_ts)) {
    session_start_new_day(now_ts);
  } else if (diff >= 10) {
    debug_log("tracker", "Large time gap detected (%u s). Assuming resume from sleep.", diff);
    session_flush_pending();
  } else {
    pending_seconds++;

    if (pending_seconds >= 60) {
      session_total_seconds += pending_seconds;
      storage_log_session(current_game_uid, session_total_seconds,
                          session_start_ts, &current_session_offset);
      debug_log("tracker", "Periodic Flush (Total: %u s)", session_total_seconds);
      pending_seconds = 0;
    }
  }

  s_last_tick_ts = now_ts;
}

static int tracker_thread_main(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  current_game_uid = 0;
  current_session_offset = -1;
  pending_seconds = 0;
  session_total_seconds = 0;
  u32 boot_ts = utils_get_timestamp();
  session_start_ts = boot_ts;

  const PluginConfigRuntime *cfg = plugin_config_get();
  int hotkey_enabled = cfg && cfg->hotkey_enabled;

  overlay_notification_init();

  sceKernelDelayThread(TRACKER_BASE_SETTLE_US);
  resolve_late_umd_metadata();

  if (!running) {
    return 0;
  }

  const GameMetadata *metadata = detector_get_metadata();
  const char *prefix = utils_get_device_prefix();
  char base_dir[128];
  snprintf(base_dir, sizeof(base_dir), "%s%s", prefix, GDIARY_BASE_DIR);

  memset(&g_runtime, 0, sizeof(g_runtime));
  tracker_runtime_init(&g_runtime, base_dir, metadata, hotkey_enabled);

  int unresolved_psp = metadata_is_unresolved_psp(metadata);

  if (unresolved_psp) {
    debug_log("tracker", "UMD metadata unresolved after %u attempts; playtime disabled.",
              (unsigned int)TRACKER_UMD_MAX_ATTEMPTS);
    if (!hotkey_enabled) {
      return 0;
    }
  } else if (tracker_runtime_is_tracking(&g_runtime)) {
    storage_ensure_initialized(base_dir);
    session_begin_tracking(metadata, 1);

    u32 now_ts = utils_get_timestamp();
    if (now_ts > boot_ts) {
      session_total_seconds = now_ts - boot_ts;
      session_start_ts = boot_ts;
      debug_log("tracker", "Backfilled startup time before tracking init (%u s)",
                (unsigned int)session_total_seconds);
    }
  } else {
    debug_log("tracker", "Tracking blocked for %s (cat %u)",
              metadata->game_id, (unsigned int)metadata->category);
  }

  /*
   * Startup notification: inform the user of the initial tracking state
   * right after the tracker.dat file is read — only when hotkey is active,
   * since without it the user has no way to interact with the state anyway.
   */
  if (hotkey_enabled) {
    if (!unresolved_psp && tracker_runtime_is_tracking(&g_runtime)) {
      overlay_notification_show("GameDiary - Tracker: ON", NULL,
                                OVERLAY_NOTIFY_TRACKER_ON);
      debug_log("tracker", "Startup notification: tracking ON for %s",
                metadata ? metadata->game_id : "?");
    } else {
      overlay_notification_show("GameDiary - Tracker: OFF", NULL,
                                OVERLAY_NOTIFY_TRACKER_OFF);
      debug_log("tracker", "Startup notification: tracking OFF for %s",
                metadata ? metadata->game_id : "?");
    }
  }

  s_last_tick_ts = utils_get_timestamp();

  while (running) {
    metadata = detector_get_metadata();
    u32 slice;
    for (slice = 0; slice < TRACKER_INPUT_SLICES_PER_SEC && running; slice++) {
      if (hotkey_enabled) {
        u32 now_ms = utils_get_time_ms();
        int ready = tracker_runtime_metadata_ready(metadata);

        if (overlay_notification_is_visible(now_ms)) {
          /* Redraw every vblank so the game does not erase the message (~60 Hz). */
          while (running && overlay_notification_is_visible(utils_get_time_ms())) {
            sceDisplayWaitVblankStart();
            overlay_notification_draw();
            now_ms = utils_get_time_ms();
            InputComboEvent ev = input_combo_poll(now_ms, ready, 1);
            handle_combo_event(ev, metadata);
          }
        } else {
          InputComboEvent ev = input_combo_poll(now_ms, ready, 1);
          handle_combo_event(ev, metadata);
          sceKernelDelayThread(TRACKER_INPUT_SLICE_US);
        }
      } else {
        sceKernelDelayThread(1000 * 1000);
        break;
      }
    }

    if (!hotkey_enabled) {
      /* single 1 s delay already done above */
    }

    u32 now_ts = utils_get_timestamp();
    playtime_tick(now_ts);
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
    storage_log_session(current_game_uid, session_total_seconds,
                        session_start_ts, &current_session_offset);
    pending_seconds = 0;
  }
  overlay_notification_shutdown();
}
