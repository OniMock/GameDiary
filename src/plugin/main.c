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
 * @file main.c
 * @brief Main plugin entry point.
 */

#include "common/common.h"
#include "common/storage.h"
#include <pspkernel.h>
#include "common/db_schema.h"
#include "plugin/apitype.h"
#include "plugin/detector.h"
#include "plugin/tracker.h"
#include "common/utils.h"
#include "common/debug.h"

int sceKernelInitApitype(void);

PSP_MODULE_INFO("GameDiary", 0x1000, 1, 0);

int module_start(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  // Check environment
  int apitype = sceKernelInitApitype();
  int cat = apitype_detect_category(apitype);

  debug_init();
  debug_log("main", "Plugin starting (APITYPE: 0x%X, CAT: %d)", apitype, cat);

  // If it's VSH, we skip loading to not track idle XMB time.
  if (cat == CAT_VSH) {
    return 1; // VSH, ignore
  }

  // Grab game info right at boot before the buffer clears
  detector_init();

  // Self-exclusion: Don't track GameDiary itself to avoid database pollution
  const GameMetadata *meta = detector_get_metadata();
  debug_log("main", "Detected Game: %s", meta->game_id);

  if (strcmp(meta->game_id, GDIARY_SELF_ID) == 0) {
    debug_log("main", "Self-detected (GameDiary), skipping tracker.");
    return 1;
  }

  // Start background tracker thread
  debug_log("main", "Starting tracker thread...");
  tracker_thread_start();

  return 0; // Success
}

int module_stop(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  tracker_thread_stop();
  return 0;
}
