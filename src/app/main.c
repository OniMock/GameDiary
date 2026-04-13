/**
 * @file main.c
 * @brief App entry point stub — placeholder for future GameDiary user-mode app.
 *
 * This file will eventually contain the UI, configuration viewer, and session
 * statistics display. For now it is intentionally minimal so the modular
 * directory structure is ready to receive future development.
 *
 * The app will read data produced by the plugin (PRX) via the common storage
 * layer (common/storage.h) — no kernel-only APIs are required here.
 */

#include "common/common.h"
#include "common/storage.h"

PSP_MODULE_INFO("GameDiaryApp", 0x0000, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

int main(void) {
  /* TODO: initialize display, load session data, render stats. */
  return 0;
}
