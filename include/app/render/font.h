/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_FONT_H
#define GAMEDIARY_FONT_H

#include <psptypes.h>
#include <stdint.h>

/**
 * @brief Initializes the font system using intraFont.
 * @return 0 on success, negative on error.
 */
int font_init(void);

/**
 * @brief Draws a string on the screen.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param str The string to draw.
 * @param color The color (0xAABBGGRR).
 * @param size Font scale/size.
 */
void font_draw_string(float x, float y, const char *str, uint32_t color, float size);

/**
 * @brief Draws a string centered horizontally.
 * @param x Center X coordinate.
 * @param y Y coordinate.
 * @param str The string to draw.
 * @param color The color.
 * @param size Font scale.
 */
void font_draw_string_centered(float x, float y, const char *str, uint32_t color, float size);

/**
 * @brief Get the width of a string in pixels at the given scale.
 */
float font_get_width(const char *str, float size);

/**
 * @brief Get the height of the font in pixels at the given scale.
 */
float font_get_height(float size);

/**
 * @brief Cleans up font resources.
 */
void font_cleanup(void);

#endif // GAMEDIARY_FONT_H
