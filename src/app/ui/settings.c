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
 * @file settings.c
 * @brief Settings screen implementation.
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/config/config.h"
#include "app/audio/audio_manager.h"
#include <pspctrl.h>
#include <stdio.h>

#include "app/ui/ui_style.h"
#include "app/ui/ui_loading.h"
#include "common/utils.h"

#define SETTINGS_MENU_COUNT 5
#define MAX_VISIBLE_ITEMS 4

static int g_selection = 0;
static int g_scroll_offset = 0;
static u32 s_loading_start_ms = 0;
static bool s_is_changing = false;
static int s_pending_action = -1; // 1: Theme, 2: SFX

static float s_anim_theme = 0.0f;
static float s_anim_sfx = 0.0f;

static const char* s_helper_lines[8];
static PopupData s_helper_data;

static void settings_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_SELECT);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_START_MENU);
    s_helper_lines[5] = "";
    s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_SETTINGS);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;
}

static void settings_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    if (pressed & PSP_CTRL_UP) {
        audio_play_sfx(SFX_NAVIGATE);
        g_selection = (g_selection - 1 + SETTINGS_MENU_COUNT) % SETTINGS_MENU_COUNT;
        
        // Scroll up if selection goes above view
        if (g_selection < g_scroll_offset) {
            g_scroll_offset = g_selection;
        } else if (g_selection == SETTINGS_MENU_COUNT - 1) {
            // Jump to bottom
            g_scroll_offset = SETTINGS_MENU_COUNT - MAX_VISIBLE_ITEMS;
            if (g_scroll_offset < 0) g_scroll_offset = 0;
        }
    }
    if (pressed & PSP_CTRL_DOWN) {
        audio_play_sfx(SFX_NAVIGATE);
        g_selection = (g_selection + 1) % SETTINGS_MENU_COUNT;
        
        // Scroll down if selection goes below view
        if (g_selection >= g_scroll_offset + MAX_VISIBLE_ITEMS) {
            g_scroll_offset = g_selection - (MAX_VISIBLE_ITEMS - 1);
        } else if (g_selection == 0) {
            // Jump to top
            g_scroll_offset = 0;
        }
    }

    if (pressed & PSP_CTRL_CROSS) {
        if (g_selection == 0) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_language_select);
        } else if (g_selection == 1) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_formatting);
        } else if (g_selection == 2) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_backup);
        } else if (g_selection == 3) {
            audio_play_sfx(SFX_CONFIRM);
            ui_loading_show(i18n_get(MSG_LOADING)); // We'll update the text next frame
            s_loading_start_ms = utils_get_time_ms();
            s_is_changing = true;
            s_pending_action = 1;
        } else if (g_selection == 4) {
            audio_play_sfx(SFX_CONFIRM);
            AppConfig* cfg = config_get();
            cfg->sfx_enabled = !cfg->sfx_enabled;
            config_save();
        }
    }

    // Handle the timed loading state machine
    if (s_is_changing) {
        u32 elapsed = utils_get_time_ms() - s_loading_start_ms;

        // Step 1: Perform the work on the first frame after show
        if (s_pending_action != -1 && elapsed > 16) {
            if (s_pending_action == 1) {
                AppConfig* cfg = config_get();
                cfg->theme = !cfg->theme;
                config_save();

                if (cfg->theme == 1) ui_style_set_light();
                else ui_style_set_dark();
            }
            s_pending_action = -1; // Work done
        }

        // Step 2: Wait for minimum duration (1000ms)
        if (s_pending_action == -1 && elapsed >= 1000) {
            ui_loading_hide();
            s_is_changing = false;
        }
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        audio_play_sfx(SFX_CANCEL);
    }
}

static void settings_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    ui_draw_title_auto(i18n_get(MSG_MENU_SETTINGS), safe_rect, &GD_IMG_ICON_SETTINGS_32_PNG);

    Rect list_area = {60, 70, 360, 160};

    int items_to_draw = (SETTINGS_MENU_COUNT < MAX_VISIBLE_ITEMS) ? SETTINGS_MENU_COUNT : MAX_VISIBLE_ITEMS;

    for (int i = 0; i < items_to_draw; i++) {
        int idx = g_scroll_offset + i;
        if (idx >= SETTINGS_MENU_COUNT) break;

        Rect item_rect = rect_column(list_area, i, MAX_VISIBLE_ITEMS, 6);

        const char* label = "";
        const ImageResource* left_icon = NULL;
        const ImageResource* right_icon = NULL;

        if (idx == 0) {
            label = i18n_get(MSG_SETTINGS_LANGUAGE);
            left_icon = &GD_IMG_ICON_LANGUAGE_32_PNG;
            right_icon = i18n_get_current_flag();
        } else if (idx == 1) {
            label = i18n_get(MSG_SETTINGS_FORMATTING);
            left_icon = &GD_IMG_ICON_FORMATTING_32_PNG;
            right_icon = NULL;
        } else if (idx == 2) {
            label = i18n_get(MSG_SETTINGS_BACKUP);
            left_icon = &GD_IMG_ICON_BACKUP_32_PNG;
            right_icon = NULL;
        } else if (idx == 3) {
            label = i18n_get(MSG_SETTINGS_THEME);
            left_icon = &GD_IMG_ICON_THEME_32_PNG;
            right_icon = NULL;
        } else if (idx == 4) {
            label = i18n_get(MSG_SETTINGS_SFX);
            left_icon = config_get()->sfx_enabled ? &GD_IMG_ICON_SOUND_ACTIVE_32_PNG : &GD_IMG_ICON_SOUND_INACTIVE_32_PNG;
            right_icon = NULL;
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         label, (idx == g_selection), left_icon, right_icon);

        if (idx == 3) {
           // Light theme is 1 (OFF), Dark theme is 0 (ON)
           bool state = (config_get()->theme == 0);
           ui_draw_toggle_switch(item_rect.x + item_rect.w - 6, item_rect.y + item_rect.h / 2, state, &s_anim_theme);
        } else if (idx == 4) {
           bool state = config_get()->sfx_enabled;
           ui_draw_toggle_switch(item_rect.x + item_rect.w - 6, item_rect.y + item_rect.h / 2, state, &s_anim_sfx);
        }
    }

    // Scrollbar
    if (SETTINGS_MENU_COUNT > MAX_VISIBLE_ITEMS) {
        Rect scroll_bg = {list_area.x + list_area.w + 10, list_area.y, 4, list_area.h};
        renderer_draw_rect(scroll_bg.x, scroll_bg.y, scroll_bg.w, scroll_bg.h, COLOR_BORDER);

        float handle_h = (float)MAX_VISIBLE_ITEMS / SETTINGS_MENU_COUNT * list_area.h;
        float handle_y = (float)g_scroll_offset / SETTINGS_MENU_COUNT * list_area.h;
        renderer_draw_rect(scroll_bg.x, list_area.y + (int)handle_y, scroll_bg.w, (int)handle_h, COLOR_ACCENT);
    }

    ui_draw_standard_hints();
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
