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
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include <pspctrl.h>
#include <stdio.h>

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

    if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE)) {
        if (g_selection == 0) {
            screen_manager_push(&g_screen_language_select);
        }
    }
}

static void settings_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    ui_draw_title_auto(i18n_get(MSG_MENU_SETTINGS), safe_rect, &GD_IMG_MENU_ICON_PNG);

    Rect list_area = {60, 70, 360, 160};

    for (int i = 0; i < SETTINGS_MENU_COUNT; i++) {
        Rect item_rect = rect_column(list_area, i, 4, 10);

        const char* label = "";
        const ImageResource* left_icon = NULL;
        const ImageResource* right_icon = NULL;

        if (i == 0) {
            label = i18n_get(MSG_SETTINGS_LANGUAGE);
            left_icon = &GD_IMG_LANGUAGE_ICON_PNG;
            right_icon = i18n_get_current_flag();
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         label, (i == g_selection), left_icon, right_icon);
    }

    ui_draw_standard_hints();
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
