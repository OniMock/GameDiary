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

#define MAX_LINE_WIDTH 512
#define MAX_VISIBLE_LINES 128


/**
 * @brief Returns UTF-8 character byte size based on first byte.
 *
 * @param c First byte of UTF-8 character.
 * @return Number of bytes (1–4).
 */
size_t utf8_char_size(unsigned char c);


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

/**
 * @brief Splits a UTF-8 string into two parts, attempting to balance visual width
 *        while prioritizing semantic breakpoints (spaces or CamelCase transitions).
 *
 * @param src       Source string (null-terminated).
 * @param out1      Buffer for the first part.
 * @param out2      Buffer for the second part.
 * @param out1_size Size of out1 buffer.
 * @param out2_size Size of out2 buffer.
 * @param text_size Font scale used for measurement.
 */
void ui_text_utf8_split_smart(const char *src,
                              char *out1, char *out2,
                              size_t out1_size, size_t out2_size,
                              float text_size);

/**
 * @brief Simple multi-line text wrapping helper.
 * 
 * @param src Source text.
 * @param scale Font scale.
 * @param max_px_width Target pixel width.
 * @param out_lines Output 2D array [MAX_VISIBLE_LINES][MAX_LINE_WIDTH].
 * @param max_lines Maximum number of lines.
 * @param out_count Actual number of lines populated.
 */
void ui_text_wrap(const char* src, float scale, int max_px_width,
                  char out_lines[][MAX_LINE_WIDTH], int max_lines, int* out_count);

#endif // GAMEDIARY_UI_TEXT_H
