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

  // We only track games (UMD, ISO, Homebrew) and POPS
  // VSH API types: 0x120 (vsh), 0x140 (game), etc.
  // POPS is usually 0x143 or 0x141 depending on launcher.
  // If it's VSH, we skip loading to not track idle XMB time.
  if (apitype == 0x120 || apitype == 0x110) {
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
