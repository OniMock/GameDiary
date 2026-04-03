#include "apitype.h"
#include "common.h"
#include "detector.h"
#include "storage.h"
#include "tracker.h"


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

  // Initialize storage directory
  storage_init();

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
