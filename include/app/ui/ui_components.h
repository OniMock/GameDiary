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

#endif // GAMEDIARY_UI_COMPONENTS_H
