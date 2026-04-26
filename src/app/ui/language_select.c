/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

 /**
  * @file language_select.c
  * @brief Language select screen implementation.
  */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/audio/audio_manager.h"
#include <pspctrl.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_VISIBLE_ITEMS 4

static int g_selection = 0;
static int g_scroll_offset = 0;

static const char* s_helper_lines[7];
static PopupData s_helper_data;

static void language_select_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_SELECT);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[4] = "";
    s_helper_lines[5] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[6] = i18n_get(MSG_HELP_DESC_LANG_SELECT);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 7;
    s_helper_data.show_close_hint = true;

    int current = config_get()->language;
    if (current == LANG_AUTO) {
        g_selection = 0;
    } else {
        /* Find the A-Z sorted visual row for the saved LanguageId.
         * We can't use the enum value directly because the sorted
         * order differs from the enum declaration order. */
        g_selection = 0; /* fallback to Auto if not found */
        for (int i = 0; i < LANG_COUNT; i++) {
            if (i18n_get_sorted_lang_index(i) == current) {
                g_selection = i + 1; /* +1: slot 0 is reserved for "Auto" */
                break;
            }
        }
    }

    /* Ensure the initial selection is within the visible window */
    if (g_selection >= MAX_VISIBLE_ITEMS) {
        g_scroll_offset = g_selection - (MAX_VISIBLE_ITEMS - 1);
    } else {
        g_scroll_offset = 0;
    }
}

static void language_select_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    int menu_count = LANG_COUNT + 1;

    if (pressed & PSP_CTRL_UP) {
        audio_play_sfx(SFX_NAVIGATE);
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
        audio_play_sfx(SFX_NAVIGATE);
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
        audio_play_sfx(SFX_CONFIRM);
        /* Map visual A-Z position back to the real LanguageId enum value */
        int target_lang = (g_selection == 0)
            ? LANG_AUTO
            : (int)i18n_get_sorted_lang_index(g_selection - 1);
        i18n_init(target_lang);
        config_get()->language = target_lang;
        config_save();
    }
}

static void language_select_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    ui_draw_title_auto(i18n_get(MSG_SETTINGS_LANGUAGE), safe_rect, &GD_IMG_ICON_LANGUAGE_32_PNG);

    int menu_count = LANG_COUNT + 1;
    Rect list_area = {60, 70, 360, 160}; // Height for 4 items (~40px each)

    int items_to_draw = (menu_count < MAX_VISIBLE_ITEMS) ? menu_count : MAX_VISIBLE_ITEMS;

    for (int i = 0; i < items_to_draw; i++) {
        int idx = g_scroll_offset + i;
        if (idx >= menu_count) break;

        Rect item_rect = rect_column(list_area, i, MAX_VISIBLE_ITEMS, 5);
        const char* name;

        if (idx == 0) {
            name = "Auto (System)";
        } else {
            /* Resolve the real LanguageId for this A-Z visual slot */
            LanguageId lang_id = i18n_get_sorted_lang_index(idx - 1);
            name = i18n_get_lang_name(lang_id);
        }

        int current_config = config_get()->language;
        /* item_lang is the real LanguageId so it can be compared to config */
        int item_lang = (idx == 0)
            ? LANG_AUTO
            : (int)i18n_get_sorted_lang_index(idx - 1);
        /* Active Language Check */
        bool is_active = (item_lang == current_config);

        const ImageResource* right_flag = NULL;

        if (idx != 0) {
            right_flag = i18n_get_lang_flag(i18n_get_sorted_lang_index(idx - 1));
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         name, (idx == g_selection), NULL, right_flag);

        // indicator for the active language if not selected
        if (is_active && idx != g_selection) {
             renderer_draw_rect(item_rect.x + item_rect.w - 5, item_rect.y + 10, 3, item_rect.h - 20, COLOR_SUCCESS);
        }
    }

    // Scrollbar
    if (menu_count > MAX_VISIBLE_ITEMS) {
        Rect scroll_bg = {list_area.x + list_area.w + 10, list_area.y, 4, list_area.h};
        renderer_draw_rect(scroll_bg.x, scroll_bg.y, scroll_bg.w, scroll_bg.h, COLOR_BORDER);

        float handle_h = (float)MAX_VISIBLE_ITEMS / menu_count * list_area.h;
        float handle_y = (float)g_scroll_offset / menu_count * list_area.h;
        renderer_draw_rect(scroll_bg.x, list_area.y + (int)handle_y, scroll_bg.w, (int)handle_h, COLOR_ACCENT);
    }

    ui_draw_standard_hints();
}

Screen g_screen_language_select = {
    language_select_init,
    language_select_update,
    language_select_draw,
    NULL
};
