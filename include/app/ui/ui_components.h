/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_UI_COMPONENTS_H
#define GAMEDIARY_UI_COMPONENTS_H

#include "app/ui/ui_layout.h"
#include "app/data/stats_calculator.h"
#include "common/db_schema.h"
#include <psptypes.h>
#include "app/render/image_resources.h"

// Theme Colors (ABGR - 0xAABBGGRR)

// --- Base ---
#define COLOR_BG        0xFF0F0F0F // Background - #0F0F0F
#define COLOR_CARD      0xFF1E1E1E // Card surface - #1E1E1E
#define COLOR_BORDER    0xFF252525 // Subtle border - #252525 (less harsh)
#define COLOR_HIGHLIGHT 0xAA444444 // Hover/selection - #444444 (more visible)

// --- Text ---
#define COLOR_TEXT      0xFFFFFFFF // Primary text - #FFFFFF
#define COLOR_SUBTEXT   0xFFBBBBBB // Secondary text - #BBBBBB (better readability)
#define COLOR_SUBTEXT2   0xAA6A6F75 // #6A6F75 (light cool gray, well balanced)

// --- Accent & Feedback ---
#define COLOR_ACCENT 0xFFFFC040 // #40C0FF
#define COLOR_SUCCESS   0xFF20B020 // Success (green) - #20B020

// --- Font Sizes (Values in pixels) ---
#define UI_FONT_SIZE_TITLE_MAIN  22.0f    // Page headers & Branding
#define UI_FONT_SIZE_TITLE_HUGE  17.0f    // Pop-up headers & Error titles
#define UI_FONT_SIZE_TITLE_LIST  16.0f    // List item primary focus
#define UI_FONT_SIZE_PRIMARY     15.0f    // Menu entries & Card titles
#define UI_FONT_SIZE_MEDIUM      14.0f    // Emphasized stats & secondary titles
#define UI_FONT_SIZE_NORMAL      13.0f    // Standard labels & UI body
#define UI_FONT_SIZE_SMALL       12.0f    // Secondary text & wrapped body
#define UI_FONT_SIZE_TINY        11.0f    // Indicators, Footers & Dates
#define UI_FONT_SIZE_NANO        10.0f    // Detailed graph data values
#define UI_FONT_SIZE_PICO        9.0f     // Micro labels for dense graphs

// --- Icon Sizes (Values in pixels) ---
#define UI_ICON_SIZE_LOGO        32
#define UI_ICON_SIZE_TITLE       24
#define UI_ICON_SIZE_MENU        24


typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } UIAlign;



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
 * Draws the standard suite of common navigation hints.
 */
void ui_draw_standard_hints(void);

/**
 * Draws a section title with underline.
 */
void ui_draw_title(const char *text, Rect r, const ImageResource *icon, int custom_icon_size);

/**
 * Draws a section title with underline.
 */
void ui_draw_title_auto(const char *text, Rect r, const ImageResource *icon);

/**
 * Draws the specialized bipartide application header (Logo + Split Title).
 * Used exclusively on the home/main menu.
 */
void ui_draw_app_header(Rect r);

/**
 * @brief Draws an animated statistical graph supporting multiple time ranges.
 * Uses smooth fade and slide transition logic.
 */
void ui_draw_stats_graph(const StatsGraphData *data, int center_x, int baseline_y, int total_w, int max_bar_h);

/**
 * Resets the graph animation state so it slides in again on next redraw.
 */
void ui_reset_stats_graph_animation(void);

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
 * Draws a bar graph showing the last `max_bars` active days of a specific game.
 * Playtime is consolidated per day (midnight-to-midnight).
 *
 * Bars are ordered oldest-left → newest-right, using a colour gradient.
 * Heights are animated frame-by-frame for a smooth grow-in effect.
 *
 * @param sessions    Pointer to the global sessions array.
 * @param count       Total number of entries in `sessions`.
 * @param game_uid    UID of the game to aggregate.
 * @param max_bars    Maximum number of bars (days) to show.
 * @param center_x    Center X of the whole graph.
 * @param baseline_y  Y coordinate of the bar baseline.
 * @param max_bar_h   Maximum bar height in pixels.
 */
void ui_draw_game_daily_graph(const SessionEntry *sessions, int count,
                              u32 game_uid, int max_bars,
                              int center_x, int baseline_y, int max_bar_h);

/**
 * Resets the game daily graph animation state.
 */
void ui_reset_game_daily_graph_animation(void);

/**
 * Formats a duration in seconds into a localized human-readable string.
 * Handles days, hours, and minutes based on value.
 */
void ui_format_duration(u32 seconds, char *out, size_t size);

/**
 * @brief Draws animated navigation indicators (◀ / ▶) at the left and right screen edges.
 *
 * @param y             Y coordinate (baseline) for the indicators.
 * @param show_left     Whether to show the left indicator.
 * @param show_right    Whether to show the right indicator.
 * @param animate_left  Whether the left indicator should pulse.
 * @param animate_right Whether the right indicator should pulse.
 * @param last_nav_ms   System timestamp of the last rapid navigation (to pause animation briefly).
 * @param color         Color to use for the indicators (e.g., COLOR_ACCENT or COLOR_TEXT).
 */
void ui_draw_nav_indicators(int y, bool show_left, bool show_right, bool animate_left, bool animate_right, u32 last_nav_ms, u32 color);

#endif // GAMEDIARY_UI_COMPONENTS_H
