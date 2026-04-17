/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_RENDERER_H
#define GAMEDIARY_RENDERER_H

#include <psptypes.h>
#include <stdint.h>

/**
 * @brief Initializes the renderer.
 */
void renderer_init(void);

/**
 * @brief Starts a new frame.
 */
void renderer_start_frame(void);

/**
 * @brief Ends the current frame.
 */
void renderer_end_frame(void);

/**
 * @brief Clears the screen with a specific color.
 * @param color The color to clear the screen with.
 */
void renderer_clear(uint32_t color);

/**
 * @brief Draws a rectangle.
 * @param x The x coordinate of the rectangle.
 * @param y The y coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param color The color of the rectangle.
 */
void renderer_draw_rect(int x, int y, int w, int h, uint32_t color);

#endif // GAMEDIARY_RENDERER_H
