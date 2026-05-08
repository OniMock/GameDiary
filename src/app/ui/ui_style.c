#include "app/ui/ui_style.h"

UIStyle g_ui_style;

void ui_style_init(void) {
    ui_style_set_dark(); // Default theme
}

void ui_style_set_dark(void) {
    // ABGR format is handled by UI_COLOR_HEX macro

    // Core Surfaces
    g_ui_style.bg        = UI_COLOR_HEX(0x0F0F0F); // #0F0F0F - Background
    g_ui_style.card      = UI_COLOR_HEX(0x1E1E1E); // #1E1E1E - Card/Surface
    g_ui_style.border    = UI_COLOR_HEX(0x252525); // #252525 - Borders/Dividers
    g_ui_style.highlight = UI_COLOR_ALPHA(UI_COLOR_HEX(0x444444), 66); // #444444 - Selection/Hover (66% alpha)

    // Typography
    g_ui_style.text      = UI_COLOR_HEX(0xFFFFFF); // #FFFFFF - Main Text
    g_ui_style.subtext   = UI_COLOR_HEX(0xBBBBBB); // #BBBBBB - Secondary Text
    g_ui_style.subtext2  = UI_COLOR_HEX(0x6A6F75); // #6A6F75 - Tertiary Text

    // Semantic Colors
    g_ui_style.accent    = UI_COLOR_HEX(0x40C0FF); // #40C0FF - Accent (blue)
    g_ui_style.success   = UI_COLOR_HEX(0x20B020); // #20B020 - Success (green)
    g_ui_style.danger    = UI_COLOR_HEX(0xB02020); // #B02020 - Danger (red)

    g_ui_style.shadow    = UI_COLOR_HEX(0x000000); // #000000 - Pure black shadow
    g_ui_style.shadow_alpha = 15;      // 15% opacity

    g_ui_style.icon_tint = UI_COLOR_HEX(0xFFFFFF); // #FFFFFF - White icons

}

void ui_style_set_light(void) {
    // Core Surfaces: "Nordic Frost" (Cool tones, ice and slate - 0% pure white)
    g_ui_style.bg        = UI_COLOR_HEX(0xD8DEE9); // #D8DEE9 - Icy blue-gray background
    g_ui_style.card      = UI_COLOR_HEX(0xE5E9F0); // #E5E9F0 - Slightly lighter frost for card surfaces
    g_ui_style.border    = UI_COLOR_HEX(0x85929E); // #85929E - Solid blue-gray for distinct borders
    g_ui_style.highlight = UI_COLOR_HEX(0xB0BDD1); // #B0BDD1 - Cornflower blue for elegant selection highlights

    // Typography (Maximum contrast for perfect readability)
    g_ui_style.text      = UI_COLOR_HEX(0x1B242E); // #1B242E - Deep navy, nearly black
    g_ui_style.subtext   = UI_COLOR_HEX(0x28323D); // #28323D - Dark slate for highly visible hints
    g_ui_style.subtext2  = UI_COLOR_HEX(0x33404D); // #33404D - Solid slate for secondary details

    // Semantic Colors (Vibrant and high-impact)
    g_ui_style.accent    = UI_COLOR_HEX(0x2563EB); // #2563EB - Vibrant Royal Blue for highlights and charts
    g_ui_style.success   = UI_COLOR_HEX(0x059669); // #059669 - Strong emerald green
    g_ui_style.danger    = UI_COLOR_HEX(0xDC2626); // #DC2626 - Vibrant Crimson Red

    // Shadows & Icons
    g_ui_style.shadow    = UI_COLOR_HEX(0x2E3440); // #2E3440 - Deep slate shadow
    g_ui_style.shadow_alpha = 15;      // 15% opacity

    g_ui_style.icon_tint = UI_COLOR_HEX(0x3B4252); // #3B4252 - Deep slate blue for icons as requested
}
