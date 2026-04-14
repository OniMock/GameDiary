#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include <pspctrl.h>
#include <stdint.h>

// Menu indices: 0 = Auto, 1 = EN, 2 = PT, 3 = ES
static int g_selection = 0;

static void settings_init(void) {
    int current = config_get()->language;
    if (current == LANG_AUTO) g_selection = 0;
    else g_selection = current + 1;
}

static void settings_update(u32 buttons, u32 pressed) {
    (void)buttons;
    int menu_count = LANG_COUNT + 1;
    
    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + menu_count) % menu_count;
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % menu_count;
    }
    if (pressed & PSP_CTRL_CROSS) {
        int target_lang = (g_selection == 0) ? LANG_AUTO : g_selection - 1;
        
        // Apply and Save
        i18n_init(target_lang);
        config_get()->language = target_lang;
        config_save();
    }
    if (pressed & PSP_CTRL_CIRCLE) {
        screen_manager_set(&g_screen_dashboard);
    }
}

static void settings_draw(void) {
    renderer_clear(COLOR_BG);
    
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);
    
    ui_draw_title(i18n_get(MSG_SETTINGS_LANGUAGE), safe_rect);
    
    int menu_count = LANG_COUNT + 1;
    Rect list_area = {60, 70, 360, 160};
    
    for (int i = 0; i < menu_count; i++) {
        Rect item_rect = rect_column(list_area, i, menu_count, 5);
        const char* name;
        
        if (i == 0) name = "Auto (System)";
        else name = i18n_get_lang_name(i - 1);
        
        if (i == g_selection) {
            ui_draw_card(item_rect, COLOR_HIGHLIGHT, COLOR_ACCENT);
        } else {
            ui_draw_card(item_rect, COLOR_CARD, COLOR_BORDER);
        }
        
        uint32_t color = (i == g_selection) ? COLOR_ACCENT : COLOR_TEXT;
        int current_config = config_get()->language;
        int item_lang = (i == 0) ? LANG_AUTO : i - 1;
        
        if (item_lang == current_config) {
            ui_draw_text(name, rect_padding(item_rect, 5), color, 0.9f, ALIGN_LEFT);
            ui_draw_text("(Active)", rect_padding(item_rect, 5), COLOR_SUBTEXT, 0.7f, ALIGN_RIGHT);
        } else {
            ui_draw_text(name, rect_padding(item_rect, 5), color, 0.9f, ALIGN_LEFT);
        }
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
