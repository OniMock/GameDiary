#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include <pspctrl.h>

#define SETTINGS_MENU_COUNT 1

static int g_selection = 0;

static void settings_init(void) {
    // Keep selection or reset
}

static void settings_update(u32 buttons, u32 pressed) {
    (void)buttons;
    
    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + SETTINGS_MENU_COUNT) % SETTINGS_MENU_COUNT;
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % SETTINGS_MENU_COUNT;
    }
    
    if (pressed & PSP_CTRL_CROSS) {
        if (g_selection == 0) {
            screen_manager_set(&g_screen_language_select);
        }
    }
    
    if (pressed & PSP_CTRL_CIRCLE) {
        screen_manager_set(&g_screen_dashboard);
    }
}

static void settings_draw(void) {
    renderer_clear(COLOR_BG);
    
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);
    
    ui_draw_title(i18n_get(MSG_MENU_SETTINGS), safe_rect);
    
    Rect list_area = {60, 70, 360, 160};
    
    for (int i = 0; i < SETTINGS_MENU_COUNT; i++) {
        Rect item_rect = rect_column(list_area, i, 4, 10); // Standard height
        
        uint32_t bg_color = (i == g_selection) ? COLOR_HIGHLIGHT : COLOR_CARD;
        uint32_t border_color = (i == g_selection) ? COLOR_ACCENT : COLOR_BORDER;
        
        ui_draw_card(item_rect, bg_color, border_color);
        
        const char* label = "";
        if (i == 0) label = i18n_get(MSG_SETTINGS_LANGUAGE);
        
        ui_draw_text(label, rect_padding(item_rect, 10), COLOR_TEXT, 1.0f, ALIGN_LEFT);
    }
    
    ui_draw_hint(i18n_get(MSG_CTRL_BACK), 20, 255, COLOR_SUBTEXT);
    ui_draw_hint(i18n_get(MSG_CTRL_SELECT), 390, 255, COLOR_SUBTEXT);
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
