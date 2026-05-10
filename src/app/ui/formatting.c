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
 * @file formatting.c
 * @brief Formatting settings screen implementation.
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

#define FORMATTING_MENU_COUNT 1

static int g_selection = 0;
static float s_anim_toggle = 0.0f;

static const char* s_helper_lines[8];
static PopupData s_helper_data;

static void formatting_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_CHANGE);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[4] = "";
    s_helper_lines[5] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[6] = i18n_get(MSG_HELP_DESC_FORMATTING);
    s_helper_lines[7] = "";

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;

    // Initialize animation state
    s_anim_toggle = config_get()->format_hours_only ? 1.0f : 0.0f;
}

static void formatting_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + FORMATTING_MENU_COUNT) % FORMATTING_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % FORMATTING_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }

    if (pressed & PSP_CTRL_CROSS) {
        if (g_selection == 0) {
            audio_play_sfx(SFX_CONFIRM);
            AppConfig* cfg = config_get();
            cfg->format_hours_only = !cfg->format_hours_only;
            config_save();
        }
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        audio_play_sfx(SFX_CANCEL);
        screen_manager_pop();
    }
}

static void formatting_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    ui_draw_title_auto(i18n_get(MSG_SETTINGS_FORMATTING), safe_rect, &GD_IMG_ICON_FORMATTING_32_PNG);

    Rect list_area = {60, 70, 360, 160};

    for (int i = 0; i < FORMATTING_MENU_COUNT; i++) {
        Rect item_rect = rect_column(list_area, i, 4, 10);

        const char* label = "";

        if (i == 0) {
            label = i18n_get(MSG_FORMATTING_HOURS_ONLY);
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         label, (i == g_selection), NULL, NULL);

        if (i == 0) {
           bool state = config_get()->format_hours_only;
           ui_draw_toggle_switch(item_rect.x + item_rect.w - 6, item_rect.y + item_rect.h / 2, state, &s_anim_toggle);
        }
    }

    ui_draw_standard_hints();
}

Screen g_screen_formatting = {
    formatting_init,
    formatting_update,
    formatting_draw,
    NULL
};
