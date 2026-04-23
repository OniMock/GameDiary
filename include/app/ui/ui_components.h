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

// --- Font Sizes (Scales relative to 17px IntraFont base) ---
#define UI_FONT_SIZE_TITLE_MAIN  1.30f    // Page headers & Branding (~22px)
#define UI_FONT_SIZE_TITLE_HUGE  1.00f    // Pop-up headers & Error titles (17px)
#define UI_FONT_SIZE_TITLE_LIST  0.95f    // List item primary focus (~16px)
#define UI_FONT_SIZE_PRIMARY     0.90f    // Menu entries & Card titles (~15px)
#define UI_FONT_SIZE_MEDIUM      0.85f    // Emphasized stats (~14px)
#define UI_FONT_SIZE_NORMAL      0.80f    // Standard labels & UI body (~13px)
#define UI_FONT_SIZE_SMALL       0.75f    // Secondary text & wrapped body (~12px)
#define UI_FONT_SIZE_COMPACT     0.72f    // Metadata line (~12px)
#define UI_FONT_SIZE_TINY        0.70f    // Indicators & Footers (~12px)
#define UI_FONT_SIZE_MICRO       0.65f    // Sidebar headers & tags (~11px)
#define UI_FONT_SIZE_NANO        0.60f    // Detailed graph data values (~10px)


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

#endif // GAMEDIARY_UI_COMPONENTS_H
