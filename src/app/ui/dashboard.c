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
 * @file dashboard.c
 * @brief Dashboard screen implementation.
 *
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include <pspgu.h>
#include <pspctrl.h>
#include <stdio.h>

static void dashboard_init(void) {
    data_calculate_stats(0, 0xFFFFFFFF);
    ui_reset_graph_animation();
}

static void dashboard_update(u32 buttons, u32 pressed) {
    (void)buttons;
    (void)pressed;
    // inputs like Triangle, Start, Select are handled globally by screen_manager
}

static void dashboard_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    // Header
    ui_draw_title_auto(i18n_get(MSG_MENU_STATS), safe_rect, &GD_IMG_ICON_STATS_32_PNG);

    // Weekly Graph (Center area)
    ui_draw_weekly_graph(data_get_sessions(), data_get_session_count());

    ui_draw_standard_hints();
}

Screen g_screen_dashboard = {
    dashboard_init,
    dashboard_update,
    dashboard_draw,
    NULL
};
