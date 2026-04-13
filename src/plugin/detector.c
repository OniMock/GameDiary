#include "plugin/detector.h"
#include "plugin/metadata_repository.h"
#include <string.h>

static GameMetadata g_current_game;

void detector_init(void) { metadata_fetch(&g_current_game); }

void detector_init_late(void) {
  // If the game info is still unknown or HBX hash, try a late fetch from UMD.
  if (strcmp(g_current_game.game_id, "UNKNOWN-00000") == 0 ||
      strncmp(g_current_game.game_id, "HBX-", 4) == 0) {
    metadata_fetch_from_umd(&g_current_game);
  }
}

const GameMetadata *detector_get_metadata(void) { return &g_current_game; }
