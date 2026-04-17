/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _COMMON_MODELS_H_
#define _COMMON_MODELS_H_

/* psptypes.h defines u8 and other PSP scalar types without pulling in
 * the full kernel API — safe for both user-mode and kernel-mode code. */
#include <psptypes.h>

/* Game category constants used throughout the plugin and common layers. */
#define CAT_PSP      0
#define CAT_PS1      1
#define CAT_HOMEBREW 2
#define CAT_VSH      3
#define CAT_UNKNOWN  4

/**
 * @brief Holds all metadata identifying a game at boot time.
 * Populated by the plugin detector and consumed by common storage.
 */
typedef struct {
  char game_id[16];    /* e.g. "ULUS-10041"       */
  char game_name[64];  /* e.g. "GTA: LCS"         */
  char apitype_str[8]; /* e.g. "0x120"            */
  u8   category;       /* One of CAT_* above      */
  char file_path[256]; /* Full path to executable */
} GameMetadata;

#endif /* _COMMON_MODELS_H_ */
