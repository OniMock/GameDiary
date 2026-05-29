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
 * @file input_combo.c
 * @brief L + R + Select hold (2 s) with release reset.
 */

#include "plugin/input_combo.h"
#include <pspctrl.h>
#include <string.h>

#define COMBO_MASK       (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT)
#define COMBO_HOLD_MS    2000U

static u32 s_hold_start_ms = 0;
static int s_holding = 0;
static int s_fired = 0;

InputComboEvent input_combo_poll(u32 now_ms, int metadata_ready, int hotkey_enabled) {
  if (!hotkey_enabled) {
    s_holding = 0;
    s_fired = 0;
    s_hold_start_ms = 0;
    return INPUT_COMBO_NONE;
  }

  SceCtrlData pad;
  memset(&pad, 0, sizeof(pad));
  sceCtrlPeekBufferPositive(&pad, 1);

  int all_down = ((pad.Buttons & COMBO_MASK) == COMBO_MASK);

  if (!all_down) {
    s_holding = 0;
    s_fired = 0;
    s_hold_start_ms = 0;
    return INPUT_COMBO_NONE;
  }

  if (!s_holding) {
    s_holding = 1;
    s_hold_start_ms = now_ms;
    s_fired = 0;
    return INPUT_COMBO_NONE;
  }

  if (s_fired) {
    return INPUT_COMBO_NONE;
  }

  if ((now_ms - s_hold_start_ms) < COMBO_HOLD_MS) {
    return INPUT_COMBO_NONE;
  }

  s_fired = 1;

  if (!metadata_ready) {
    return INPUT_COMBO_NOT_READY;
  }

  return INPUT_COMBO_TOGGLE;
}
