/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_UI_POPUP_H
#define GAMEDIARY_UI_POPUP_H

/**
 * @file ui_popup.h
 * @brief Reusable modal popup component.
 *
 * Implements a Clean Architecture approach:
 * - Domain: PopupData state
 * - Application: open, close, update logic (input hijacking)
 * - Presentation: Rendering full width boxes and lists
 */

#include "app/render/image_resources.h"
#include <stdint.h>

/**
 * @brief Domain structure for a Popup.
 */
typedef struct {
    const char* title;
    const ImageResource* icon; // Left header icon (NULL for none)
    const char** lines;        // Array of input strings (can be long strings)
    int line_count;            // Number of input strings
} PopupData;

/**
 * @brief Opens a popup and takes over the screen input processing.
 * Disables interacting with underlying screens.
 * Text lines will be wrapped dynamically.
 * @param data Pre-populated PopupData. The pointers must remain valid while open.
 */
void popup_open(const PopupData* data);

/**
 * @brief Closes the currently open popup.
 */
void popup_close(void);

/**
 * @brief Returns 1 if a popup is currently open and active, else 0.
 */
int popup_is_open(void);

/**
 * @brief Processes input solely for the popup.
 * Handles Up/Down for scrolling and Cross/Circle for closing.
 * @param buttons Buttons currently held mask.
 * @param pressed Buttons pressed this frame (edge mask).
 */
void popup_update(uint32_t buttons, uint32_t pressed);

/**
 * @brief Renders the popup overlay, box, header and text content.
 */
void popup_render(void);

/**
 * @brief Gets the current frame's sanitized pad data.
 * Used to ensure screens don't poll hardware directly while modal is active.
 */
const struct SceCtrlData* ui_get_pad(void);

/**
 * @brief Gets the current frame's sanitized pressed buttons.
 */
uint32_t ui_get_pressed(void);

#endif /* GAMEDIARY_UI_POPUP_H */
