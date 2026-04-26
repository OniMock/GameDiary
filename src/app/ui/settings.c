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

#define SETTINGS_MENU_COUNT 4

static int g_selection = 0;

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
        g_selection = (g_selection - 1 + SETTINGS_MENU_COUNT) % SETTINGS_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % SETTINGS_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }

    if (pressed & PSP_CTRL_CROSS) {
        if (g_selection == 0) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_language_select);
        } else if (g_selection == 1) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_about);
        } else if (g_selection == 2) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(&g_screen_support);
        } else if (g_selection == 3) {
            AppConfig* cfg = config_get();
            cfg->sfx_enabled = !cfg->sfx_enabled;
            config_save();
            audio_play_sfx(SFX_CONFIRM); // Plays if we just turned it ON
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

    for (int i = 0; i < SETTINGS_MENU_COUNT; i++) {
        Rect item_rect = rect_column(list_area, i, 4, 10);

        const char* label = "";
        const ImageResource* left_icon = NULL;
        const ImageResource* right_icon = NULL;

        if (i == 0) {
            label = i18n_get(MSG_SETTINGS_LANGUAGE);
            left_icon = &GD_IMG_ICON_LANGUAGE_32_PNG;
            right_icon = i18n_get_current_flag();
        } else if (i == 1) {
            label = i18n_get(MSG_SETTINGS_ABOUT);
            left_icon = &GD_IMG_ICON_ABOUT_32_PNG;
        } else if (i == 2) {
            label = i18n_get(MSG_SETTINGS_SUPPORT);
            left_icon = &GD_IMG_ICON_SUPPORT_32_PNG;
        } else if (i == 3) {
            label = i18n_get(MSG_SETTINGS_SFX);
            left_icon = config_get()->sfx_enabled ? &GD_IMG_ICON_SOUND_ACTIVE_32_PNG : &GD_IMG_ICON_SOUND_INACTIVE_32_PNG;
            // TODO: Add ON/OFF icons
            right_icon = NULL;
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         label, (i == g_selection), left_icon, right_icon);

        if (i == 3) {
           // Draw text "On" / "Off" inside the item_rect (aligned right)
           const char* status = config_get()->sfx_enabled ? i18n_get(MSG_SFX_ON) : i18n_get(MSG_SFX_OFF);
           // We use item_rect with 16px right padding for consistent alignment
           Rect status_rect = { item_rect.x, item_rect.y, item_rect.w - 16, item_rect.h };
           ui_draw_text(status, status_rect, 0xFFFFFFFF, 14.0f, ALIGN_RIGHT);
        }
    }

    ui_draw_standard_hints();
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
