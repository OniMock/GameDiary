#ifndef GAMEDIARY_UI_STYLE_H
#define GAMEDIARY_UI_STYLE_H

#include <psptypes.h>

/**
 * Helper macro to calculate alpha.
 * Pass a percentage (0-100) to get the corresponding 0-255 value.
 */
#define UI_ALPHA(pct) ((u8)(((pct) * 255) / 100))

/**
 * Helper macro to set the alpha channel of a color.
 * Overwrites the highest 8 bits (ABGR -> A).
 */
#define UI_COLOR_ALPHA(color, pct) ((((u32)UI_ALPHA(pct)) << 24) | ((color) & 0x00FFFFFF))

/**
 * Helper macro to inject an existing alpha value (0-255) into a color.
 */
#define UI_COLOR_ALPHA_VAL(color, alpha_val) ((((u32)(alpha_val)) << 24) | ((color) & 0x00FFFFFF))

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
#define UI_ICON_SIZE_TINY        16    // Small indicators
#define UI_ICON_SIZE_SMALL       24    // Title icons, menu left icons
#define UI_ICON_SIZE_MEDIUM      32    // Logo, large buttons
#define UI_ICON_SIZE_LARGE       48    // Featured icons
#define UI_ICON_SIZE_MAIN_MENU   80    // Main menu icons
#define UI_ICON_SIZE_CAROUSEL_W  144   // Focused carousel icon width
#define UI_ICON_SIZE_CAROUSEL_H  80    // Focused carousel icon height
#define UI_ICON_SIZE_SIDE_W      80    // Side carousel icon width
#define UI_ICON_SIZE_SIDE_H      45    // Side carousel icon height
#define UI_ICON_SIZE_QR          96    // Donation/Support QR codes
#define UI_ICON_SIZE_HUGE        128   // Raw resource size for large icons

#define UI_ICON_SIZE_TITLE       UI_ICON_SIZE_SMALL
#define UI_ICON_SIZE_MENU        UI_ICON_SIZE_SMALL
#define UI_ICON_SIZE_LOGO        UI_ICON_SIZE_MEDIUM

// Theme struct definition
typedef struct {
    u32 bg;
    u32 card;
    u32 border;
    u32 highlight;
    
    u32 text;
    u32 subtext;
    u32 subtext2;
    
    u32 accent;
    u32 success;
    u32 danger;
} UIStyle;

extern UIStyle g_ui_style;

// Theme Colors mapped to the current global style
#define COLOR_BG        (g_ui_style.bg)
#define COLOR_CARD      (g_ui_style.card)
#define COLOR_BORDER    (g_ui_style.border)
#define COLOR_HIGHLIGHT (g_ui_style.highlight)

#define COLOR_TEXT      (g_ui_style.text)
#define COLOR_SUBTEXT   (g_ui_style.subtext)
#define COLOR_SUBTEXT2  (g_ui_style.subtext2)

#define COLOR_ACCENT    (g_ui_style.accent)
#define COLOR_SUCCESS   (g_ui_style.success)
#define COLOR_DANGER    (g_ui_style.danger)

void ui_style_init(void);
void ui_style_set_dark(void);
void ui_style_set_light(void);

#endif // GAMEDIARY_UI_STYLE_H
