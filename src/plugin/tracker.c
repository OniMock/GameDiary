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

static void session_end_tracking(void) {
  session_flush_pending();
  current_game_uid = 0;
  current_session_offset = -1;
  session_total_seconds = 0;
  pending_seconds = 0;
}

static int session_begin_tracking(const GameMetadata *metadata, int reset_session_start) {
  if (!metadata || !g_runtime.storage_ready) {
    return -1;
  }

  utils_set_log_context(metadata->game_id);

  int st_res = storage_get_or_create_game(metadata, &current_game_uid);
  if (st_res < 0) {
    debug_log("tracker", "storage_get_or_create_game FAILED (res: %d)", st_res);
    utils_log_error("tracker", "storage_get_or_create_game failed", st_res);
    current_game_uid = 0;
    return st_res;
  }

  debug_log("tracker", "Tracking Session Initialized (UID: %u, ID: %s)",
            current_game_uid, metadata->game_id);

  if (reset_session_start) {
    session_start_ts = utils_get_timestamp();
  }
  session_total_seconds = 0;
  current_session_offset = -1;
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
    overlay_notification_show("GameDiary - Tracker not ready",
                              "GameDiary - Please wait...");
    return;
  }

  if (ev != INPUT_COMBO_TOGGLE || !metadata) {
    return;
  }

  int was_tracking = tracker_runtime_is_tracking(&g_runtime);

  if (!tracker_runtime_toggle(&g_runtime, metadata)) {
    return;
  }

  if (was_tracking) {
    session_end_tracking();
    overlay_notification_show("GameDiary - Tracker: OFF", NULL);
    debug_log("tracker", "User disabled tracking for %s (cat %u)",
              metadata->game_id, (unsigned int)metadata->category);
  } else {
    const char *prefix = utils_get_device_prefix();
    char base_dir[128];
    snprintf(base_dir, sizeof(base_dir), "%s%s", prefix, GDIARY_BASE_DIR);
    storage_ensure_initialized(base_dir);
    if (session_begin_tracking(metadata, 1) == 0) {
      overlay_notification_show("GameDiary - Tracker: ON", NULL);
      debug_log("tracker", "User enabled tracking for %s (cat %u)",
                metadata->game_id, (unsigned int)metadata->category);
    }
  }
}

static void playtime_tick(u32 now_ts, u32 *last_tick_ts) {
  if (!tracker_runtime_is_tracking(&g_runtime) || current_game_uid == 0) {
    *last_tick_ts = now_ts;
    return;
  }

  u32 diff = now_ts - *last_tick_ts;

  if (diff >= 10) {
    debug_log("tracker", "Large time gap detected (%u s). Assuming resume from sleep.", diff);

    session_total_seconds += pending_seconds;
    pending_seconds = 0;
    if (session_total_seconds > 0) {
      storage_log_session(current_game_uid, session_total_seconds,
                          session_start_ts, &current_session_offset);
      debug_log("tracker", "Pre-sleep flush (Total: %u s)", session_total_seconds);
    }

    int local_offset = utils_get_timezone_offset_seconds();
    u32 start_day = (session_start_ts + local_offset) / 86400;
    u32 current_day = (now_ts + local_offset) / 86400;

    if (start_day != current_day) {
      debug_log("tracker", "Day changed during sleep (start_day=%u, current_day=%u). Starting new session.",
                (unsigned int)start_day, (unsigned int)current_day);
      session_start_ts = now_ts;
      session_total_seconds = 0;
      current_session_offset = -1;
    }
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

  *last_tick_ts = now_ts;
}

static int tracker_thread_main(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  current_game_uid = 0;
  current_session_offset = -1;
  pending_seconds = 0;
  session_total_seconds = 0;
  session_start_ts = utils_get_timestamp();

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
    session_begin_tracking(metadata, 0);

    u32 now_ts = utils_get_timestamp();
    if (now_ts > session_start_ts) {
      session_total_seconds = now_ts - session_start_ts;
      debug_log("tracker", "Backfilled startup time before tracking init (%u s)",
                (unsigned int)session_total_seconds);
    }
  } else {
    debug_log("tracker", "Tracking blocked for %s (cat %u)",
              metadata->game_id, (unsigned int)metadata->category);
  }

  u32 last_tick_ts = utils_get_timestamp();

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
    playtime_tick(now_ts, &last_tick_ts);
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
