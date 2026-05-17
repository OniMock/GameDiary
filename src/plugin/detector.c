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
 * @file detector.c
 * @brief Game metadata detection implementation.
 */

#include "plugin/detector.h"
#include "plugin/metadata_repository.h"
#include "common/debug.h"
#include <string.h>

static GameMetadata g_current_game;

void detector_init(void) {
  debug_log("DETECTOR", "detector_init: Initiating first-stage metadata fetch.");
  metadata_fetch(&g_current_game);
  debug_log("DETECTOR", "detector_init: Fetch finished. Result -> ID: '%s', Name: '%s', Category: %d, Path: '%s'",
            g_current_game.game_id, g_current_game.game_name, g_current_game.category, g_current_game.file_path);
}

void detector_init_late(void) {
  debug_log("DETECTOR", "detector_init_late: Checking if ID needs late UMD fetch. Current ID: '%s'", g_current_game.game_id);
  // If the game info is still unknown, try a late fetch from UMD.
  // DO NOT fetch if it's an HBX- hash, as that indicates a successfully 
  // resolved Homebrew. Fetching from UMD on a homebrew will read the 
  // CFW's fake mounted UMD (which causes the UCJS10041 spoofing bug).
  if (strcmp(g_current_game.game_id, "UNKNOWN-00000") == 0) {
    debug_log("DETECTOR", "detector_init_late: ID is UNKNOWN. Attempting UMD fetch...");
    int success = metadata_fetch_from_umd(&g_current_game);
    if (success) {
      debug_log("DETECTOR", "detector_init_late: UMD fetch SUCCEEDED -> ID: '%s', Name: '%s'",
                g_current_game.game_id, g_current_game.game_name);
    } else {
      debug_log("DETECTOR", "detector_init_late: UMD fetch FAILED. Metadata remains UNKNOWN.");
    }
  } else {
    debug_log("DETECTOR", "detector_init_late: ID is already resolved. Skipping late fetch.");
  }
}

const GameMetadata *detector_get_metadata(void) { return &g_current_game; }

