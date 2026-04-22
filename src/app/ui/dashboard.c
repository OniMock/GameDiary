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
#include "app/data/stats_calculator.h"
#include <pspgu.h>
#include <pspctrl.h>
#include <stdio.h>

static StatsQuery g_current_query = {STATS_PERIOD_WEEKLY, 0};
static StatsGraphData g_cached_graph_data;

static void dashboard_set_mode(StatsPeriod mode) {
    g_current_query.period = mode;

    // Explicitly calculate data and cache it so draw loop stays at 60fps
    stats_calc_query(data_get_sessions(), data_get_session_count(), g_current_query, &g_cached_graph_data);
    ui_reset_stats_graph_animation();
}

static void dashboard_init(void) {
    data_calculate_stats(0, 0xFFFFFFFF);
    dashboard_set_mode(STATS_PERIOD_WEEKLY);
}

static void dashboard_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LEFT) {
        StatsPeriod next = (g_current_query.period + STATS_PERIOD_COUNT - 1) % STATS_PERIOD_COUNT;
        dashboard_set_mode(next);
    } else if (pressed & PSP_CTRL_RIGHT) {
        StatsPeriod next = (g_current_query.period + 1) % STATS_PERIOD_COUNT;
        dashboard_set_mode(next);
    }
}

static void dashboard_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    // Dynamic Title combining base feature string with mode string
    char title_buf[64];
    const char *mode_str = "";
    if (g_current_query.period == STATS_PERIOD_WEEKLY) mode_str = i18n_get(MSG_STATS_MODE_WEEKLY);
    else if (g_current_query.period == STATS_PERIOD_MONTHLY) mode_str = i18n_get(MSG_STATS_MODE_MONTHLY);
    else if (g_current_query.period == STATS_PERIOD_YEARLY) mode_str = i18n_get(MSG_STATS_MODE_YEARLY);

    // Mode title with arrows for indication
    snprintf(title_buf, sizeof(title_buf), "%s: %s", i18n_get(MSG_MENU_STATS), mode_str);

    // Header
    ui_draw_title_auto(title_buf, safe_rect, &GD_IMG_ICON_STATS_32_PNG);

    // Cached Graph Area
    ui_draw_stats_graph(&g_cached_graph_data, 240, 190, 360, 80);

    // Minor hint below graph indicating mode change
    Rect mode_hint_rect = {0, 230, 480, 20};
    ui_draw_text(i18n_get(MSG_HINT_CHANGE_MODE), mode_hint_rect, COLOR_SUBTEXT2, 0.8f, ALIGN_CENTER);

    ui_draw_standard_hints();
}

Screen g_screen_dashboard = {
    dashboard_init,
    dashboard_update,
    dashboard_draw,
    NULL
};
