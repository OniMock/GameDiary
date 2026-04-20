/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_UI_TEXT_H
#define GAMEDIARY_UI_TEXT_H

#include "app/ui/ui_components.h"
#include <stddef.h>

/**
 * @brief Truncates a string with an ellipsis ("...") if it exceeds a maximum width.
 * 
 * Uses an optimized binary search to find the cutoff point based on font metrics.
 *
 * @param src       Source string.
 * @param dst       Destination buffer for the truncated string.
 * @param dst_size  Size of the destination buffer.
 * @param scale     Font scale used for measurement.
 * @param max_width Maximum allowed width in pixels.
 */
void ui_text_ellipsis(const char *src, char *dst, size_t dst_size, float scale,
                      float max_width);

/**
 * @brief Draws text using an "Auto-Fit" strategy:
 *        1. Try at initial scale.
 *        2. If overflows, shrink proportionally down to 0.8 * initial scale.
 *        3. If still overflows at 0.8, truncate with ellipsis at 0.8 scale.
 *
 * @param text   Text to draw.
 * @param r      Rectangle area.
 * @param color  Text color.
 * @param size   Initial font scale.
 * @param align  Alignment.
 */
void ui_draw_text_auto_fit(const char *text, Rect r, u32 color, float size,
                           UIAlign align);

#endif // GAMEDIARY_UI_TEXT_H
