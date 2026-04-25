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

#include "plugin/apitype.h"
#include "plugin/detector.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "plugin/tracker.h"
#include "common/utils.h"
#include <pspsdk/systemctrl.h>

PSP_MODULE_INFO("GameDiary", 0x1000, 1, 0);

int module_start(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  // Check environment
  int apitype = sceKernelInitApitype();
  int cat = apitype_detect_category(apitype);

  // If it's VSH, we skip loading to not track idle XMB time.
  if (cat == CAT_VSH) {
    return 1; // VSH, ignore
  }

  /* Resolve base path securely avoiding buggy CFW sctrl hooks on PRO-C.
   * utils_get_device_prefix physically checks if ef0: exists. */
  const char *prefix = utils_get_device_prefix();
  char base_dir[128];
  snprintf(base_dir, sizeof(base_dir), "%s%s", prefix, GDIARY_BASE_DIR);

  storage_init(base_dir);

  // Grab game info right at boot before the buffer clears
  detector_init();

  // Start background tracker thread
  tracker_thread_start();

  return 0; // Success
}

int module_stop(SceSize args, void *argp) {
  (void)args;
  (void)argp;

  tracker_thread_stop();
  return 0;
}
