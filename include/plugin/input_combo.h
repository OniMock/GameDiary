/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_INPUT_COMBO_H_
#define _PLUGIN_INPUT_COMBO_H_

#include <psptypes.h>

typedef enum {
  INPUT_COMBO_NONE = 0,
  INPUT_COMBO_NOT_READY,
  INPUT_COMBO_TOGGLE
} InputComboEvent;

/**
 * @param metadata_ready  Game ID resolved enough to persist toggle.
 * @param hotkey_enabled  From plugin.dat; if 0, returns NONE immediately.
 */
InputComboEvent input_combo_poll(u32 now_ms, int metadata_ready, int hotkey_enabled);

#endif /* _PLUGIN_INPUT_COMBO_H_ */
