#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include <pspctrl.h>
#include <string.h>

static int g_selection = 0;

static void settings_init(void) {
    const char* current = i18n_current_lang();
    int count = i18n_get_lang_count();
    for (int i = 0; i < count; i++) {
        const LanguagePack* lp = i18n_get_lang_pack(i);
        if (strcmp(lp->lang_code, current) == 0) {
            g_selection = i;
            break;
        }
    }
}

static void settings_update(u32 buttons, u32 pressed) {
    (void)buttons;
    int count = i18n_get_lang_count();
    
    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + count) % count;
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % count;
    }
    if (pressed & PSP_CTRL_CROSS) {
        const LanguagePack* lp = i18n_get_lang_pack(g_selection);
        i18n_set_language(lp->lang_code);
        
        // Save to config
        strncpy(config_get()->language, lp->lang_code, 7);
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
    
    ui_draw_title(i18n_get("settings.language"), safe_rect);
    
    int count = i18n_get_lang_count();
    Rect list_area = {60, 80, 360, 140};
    
    for (int i = 0; i < count; i++) {
        const LanguagePack* lp = i18n_get_lang_pack(i);
        Rect item_rect = rect_column(list_area, i, count, 5);
        
        if (i == g_selection) {
            ui_draw_card(item_rect, COLOR_HIGHLIGHT, COLOR_ACCENT);
        } else {
            ui_draw_card(item_rect, COLOR_CARD, COLOR_BORDER);
        }
        
        uint32_t color = (i == g_selection) ? COLOR_ACCENT : COLOR_TEXT;
        if (strcmp(lp->lang_code, i18n_current_lang()) == 0) {
            // Current language indicator
            ui_draw_text(lp->lang_name, rect_padding(item_rect, 5), color, 0.9f, ALIGN_LEFT);
            ui_draw_text("(Active)", rect_padding(item_rect, 5), COLOR_SUBTEXT, 0.7f, ALIGN_RIGHT);
        } else {
            ui_draw_text(lp->lang_name, rect_padding(item_rect, 5), color, 0.9f, ALIGN_LEFT);
        }
    }
    
    ui_draw_hint(i18n_get("ctrl.back"), 20, 255, COLOR_SUBTEXT);
    ui_draw_hint(i18n_get("ctrl.select"), 390, 255, COLOR_SUBTEXT);
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
