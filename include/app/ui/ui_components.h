#ifndef GAMEDIARY_UI_COMPONENTS_H
#define GAMEDIARY_UI_COMPONENTS_H

#include "app/ui/ui_layout.h"
#include "common/db_schema.h"
#include <psptypes.h>

// Theme Colors (ARGB)
#define COLOR_BG 0xFF0F0F0F
#define COLOR_CARD 0xFF1E1E1E
#define COLOR_BORDER 0xFF2A2A2A
#define COLOR_TEXT 0xFFFFFFFF
#define COLOR_SUBTEXT 0xFFAAAAAA
#define COLOR_ACCENT 0xFFFFB020 // Soft Orange
#define COLOR_HIGHLIGHT 0xFF3A3A3A
#define COLOR_SUCCESS 0xFF20B020 // Green for active items

typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } UIAlign;

#include "app/render/image_resources.h"

/**
 * Draws a stylized card.
 */
void ui_draw_card(Rect r, u32 bg_color, u32 border_color);

/**
 * Draws text handled by the layout system.
 */
void ui_draw_text(const char *text, Rect r, u32 color, float size,
                  UIAlign align);

/**
 * Draws a button hint (e.g., "[X] Select").
 */
void ui_draw_hint(const char *text, int x, int y, u32 color);

/**
 * Draws a button hint in footer (e.g., "[X] Select").
 */
void ui_draw_hint_footer(const char *text, int x, u32 color);

/**
 * Draws a section title with underline.
 */
void ui_draw_title(const char *text, Rect r, const ImageResource *icon, int custom_icon_size);

/**
 * Draws a section title with underline.
 */
void ui_draw_title_auto(const char *text, Rect r, const ImageResource *icon);

/**
 * Draws an animated weekly activity graph.
 */
void ui_draw_weekly_graph(SessionEntry *sessions, int count);

/**
 * Resets the graph animation state so it plays again on next redraw.
 */
void ui_reset_graph_animation(void);

/**
 * Draws a standard menu item with optional icons on left and right.
 * Icons are tinted based on selection state.
 */
void ui_draw_menu_item(int x, int y, int w, int h, const char *label,
                       bool selected, const ImageResource *left_icon,
                       const ImageResource *right_icon, int custom_icon_size);

/**
 * Draws a standard menu item with optional icons on left and right.
 * Icons are tinted based on selection state.
 */
void ui_draw_menu_item_auto(int x, int y, int w, int h, const char *label,
                            bool selected, const ImageResource *left_icon,
                            const ImageResource *right_icon);

/**
 * Draws a bar graph showing the last `max_bars` sessions of a specific game.
 *
 * Bars are ordered oldest-left → newest-right, using a colour gradient
 * (dim accent for old, full accent for the most recent session).
 * Heights are animated frame-by-frame with a simple lerp for a smooth
 * grow-in effect on first display.
 *
 * Layout is self-contained: the function centers the graph horizontally
 * within the 480-px screen and uses the provided baseline_y for alignment.
 *
 * @param sessions    Pointer to the global sessions array.
 * @param count       Total number of entries in `sessions`.
 * @param game_uid    UID of the game whose sessions should be displayed.
 * @param max_bars    Maximum number of bars (clamped to 8 internally).
 * @param baseline_y  Y coordinate of the bar baseline (bottom of bars).
 * @param max_bar_h   Maximum bar height in pixels.
 */
void ui_draw_session_bar_graph(const SessionEntry *sessions, int count,
                               u32 game_uid, int max_bars,
                               int center_x, int baseline_y, int max_bar_h);

/**
 * Resets the session bar graph animation so it re-plays on the next draw.
 * Call from the screen's init() or whenever the selected game changes.
 */
void ui_reset_session_graph_animation(void);

/**
 * Formats a duration in seconds into a localized human-readable string.
 * Handles days, hours, and minutes based on value.
 */
void ui_format_duration(u32 seconds, char *out, size_t size);

#endif // GAMEDIARY_UI_COMPONENTS_H
