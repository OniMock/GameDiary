#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include <pspctrl.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_VISIBLE_ITEMS 4

static int g_selection = 0;
static int g_scroll_offset = 0;

static void language_select_init(void) {
    int current = config_get()->language;
    if (current == LANG_AUTO) g_selection = 0;
    else g_selection = current + 1;
    
    // Ensure selection is visible
    if (g_selection >= MAX_VISIBLE_ITEMS) {
        g_scroll_offset = g_selection - (MAX_VISIBLE_ITEMS - 1);
    } else {
        g_scroll_offset = 0;
    }
}

static void language_select_update(u32 buttons, u32 pressed) {
    (void)buttons;
    int menu_count = LANG_COUNT + 1;
    
    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + menu_count) % menu_count;
        // Scroll up if selection goes above view
        if (g_selection < g_scroll_offset) {
            g_scroll_offset = g_selection;
        } else if (g_selection == menu_count - 1) {
            // Jump to bottom
            g_scroll_offset = menu_count - MAX_VISIBLE_ITEMS;
            if (g_scroll_offset < 0) g_scroll_offset = 0;
        }
    }
    
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % menu_count;
        // Scroll down if selection goes below view
        if (g_selection >= g_scroll_offset + MAX_VISIBLE_ITEMS) {
            g_scroll_offset = g_selection - (MAX_VISIBLE_ITEMS - 1);
        } else if (g_selection == 0) {
            // Jump to top
            g_scroll_offset = 0;
        }
    }
    
    if (pressed & PSP_CTRL_CROSS) {
        int target_lang = (g_selection == 0) ? LANG_AUTO : g_selection - 1;
        i18n_init(target_lang);
        config_get()->language = target_lang;
        config_save();
    }
    
    if (pressed & PSP_CTRL_CIRCLE) {
        screen_manager_set(&g_screen_settings);
    }
}

static void language_select_draw(void) {
    renderer_clear(COLOR_BG);
    
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);
    
    ui_draw_title(i18n_get(MSG_SETTINGS_LANGUAGE), safe_rect);
    
    int menu_count = LANG_COUNT + 1;
    Rect list_area = {60, 70, 360, 160}; // Height for 4 items (~40px each)
    
    int items_to_draw = (menu_count < MAX_VISIBLE_ITEMS) ? menu_count : MAX_VISIBLE_ITEMS;
    
    for (int i = 0; i < items_to_draw; i++) {
        int idx = g_scroll_offset + i;
        if (idx >= menu_count) break;
        
        Rect item_rect = rect_column(list_area, i, MAX_VISIBLE_ITEMS, 5);
        const char* name;
        
        if (idx == 0) name = "Auto (System)";
        else name = i18n_get_lang_name(idx - 1);
        
        int current_config = config_get()->language;
        int item_lang = (idx == 0) ? LANG_AUTO : idx - 1;
        
        uint32_t bg_color = COLOR_CARD;
        uint32_t border_color = COLOR_BORDER;
        
        // Active Language Color (Success Green)
        if (item_lang == current_config) {
            bg_color = COLOR_SUCCESS;
        }
        
        // Selection Highlight (Accent Border)
        if (idx == g_selection) {
            border_color = COLOR_ACCENT;
            if (bg_color == COLOR_CARD) bg_color = COLOR_HIGHLIGHT;
        }
        
        ui_draw_card(item_rect, bg_color, border_color);
        ui_draw_text(name, rect_padding(item_rect, 8), COLOR_TEXT, 0.9f, ALIGN_LEFT);
    }
    
    // Scrollbar
    if (menu_count > MAX_VISIBLE_ITEMS) {
        Rect scroll_bg = {list_area.x + list_area.w + 10, list_area.y, 4, list_area.h};
        renderer_draw_rect(scroll_bg.x, scroll_bg.y, scroll_bg.w, scroll_bg.h, COLOR_BORDER);
        
        float handle_h = (float)MAX_VISIBLE_ITEMS / menu_count * list_area.h;
        float handle_y = (float)g_scroll_offset / menu_count * list_area.h;
        renderer_draw_rect(scroll_bg.x, list_area.y + (int)handle_y, scroll_bg.w, (int)handle_h, COLOR_ACCENT);
    }
    
    ui_draw_hint(i18n_get(MSG_CTRL_BACK), 20, 255, COLOR_SUBTEXT);
    ui_draw_hint(i18n_get(MSG_CTRL_SELECT), 390, 255, COLOR_SUBTEXT);
}

Screen g_screen_language_select = {
    language_select_init,
    language_select_update,
    language_select_draw,
    NULL
};
