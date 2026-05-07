#include "app/ui/ui_style.h"

UIStyle g_ui_style;

void ui_style_init(void) {
    ui_style_set_dark(); // Default theme
}

void ui_style_set_dark(void) {
    // ABGR format (0xAABBGGRR)
    g_ui_style.bg        = 0xFF0F0F0F; // Background - #0F0F0F
    g_ui_style.card      = 0xFF1E1E1E; // Card surface - #1E1E1E
    g_ui_style.border    = 0xFF252525; // Subtle border - #252525
    g_ui_style.highlight = 0xAA444444; // Hover/selection - #444444
    
    g_ui_style.text      = 0xFFFFFFFF; // Primary text - #FFFFFF
    g_ui_style.subtext   = 0xFFBBBBBB; // Secondary text - #BBBBBB
    g_ui_style.subtext2  = 0xAA6A6F75; // Light cool gray - #6A6F75
    
    g_ui_style.accent    = 0xFFFFC040; // Accent - #40C0FF
    g_ui_style.success   = 0xFF20B020; // Success (green) - #20B020
    g_ui_style.danger    = 0xFF2020B0; // Danger (red) - #B02020
}

void ui_style_set_light(void) {
    // ABGR format (0xAABBGGRR)
    g_ui_style.bg        = 0xFFF0F0F0;
    g_ui_style.card      = 0xFFFFFFFF;
    g_ui_style.border    = 0xFFDDDDDD;
    g_ui_style.highlight = 0xAADDDDDD;
    
    g_ui_style.text      = 0xFF111111;
    g_ui_style.subtext   = 0xFF555555;
    g_ui_style.subtext2  = 0xAA888888;
    
    g_ui_style.accent    = 0xFFFF9000;
    g_ui_style.success   = 0xFF209020;
    g_ui_style.danger    = 0xFF2020D0;
}
