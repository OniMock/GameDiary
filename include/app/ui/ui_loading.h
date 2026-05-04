/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_UI_LOADING_H
#define GAMEDIARY_UI_LOADING_H

/**
 * @file ui_loading.h
 * @brief Animated loading overlay popup component.
 *
 * Architecture (Clean Architecture):
 *   Domain      : ui_loading_show() / ui_loading_hide() — intent layer
 *   Application : ui_loading_update()  — state machine + animation tick
 *   Presentation: ui_loading_render()  — pure drawing, no logic
 *
 * Usage example:
 *   ui_loading_show("Loading...");
 *   // ... do work ...
 *   ui_loading_hide();
 *
 *   // Per-frame loop:
 *   ui_loading_update();
 *   ui_loading_render();  // call AFTER all scene rendering
 */

#include <stdint.h>

/**
 * @brief Shows the loading popup with the given label text.
 *
 * The popup fades in automatically.
 * The caller must keep `label` valid until ui_loading_hide() is called.
 *
 * @param label  Text shown to the right of the spinner, e.g. "Loading..."
 */
void ui_loading_show(const char *label);

/**
 * @brief Requests the loading popup to fade out and close.
 *
 * The popup will not disappear instantly — it fades out over a few frames.
 * ui_loading_is_active() returns 1 until the fade completes.
 */
void ui_loading_hide(void);

/**
 * @brief Returns 1 while the loading popup is visible or fading.
 */
int ui_loading_is_active(void);

/**
 * @brief Advances the animation state machine (fade + spinner tick).
 *
 * Call exactly once per frame, before ui_loading_render().
 */
void ui_loading_update(void);

/**
 * @brief Renders the dim overlay and the loading popup card.
 *
 * Must be called LAST in the frame render order so it appears on top.
 * This function is a pure presentation layer: no state changes.
 */
void ui_loading_render(void);

#endif /* GAMEDIARY_UI_LOADING_H */
