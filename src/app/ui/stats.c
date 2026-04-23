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
#include "app/ui/ui_popup.h"
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

static const char* s_helper_lines[4];
static PopupData s_helper_data;

static void stats_set_mode(StatsPeriod mode) {
    g_current_query.period = mode;

    // Explicitly calculate data and cache it so draw loop stays at 60fps
    stats_calc_query(data_get_sessions(), data_get_session_count(), g_current_query, &g_cached_graph_data);
    ui_reset_stats_graph_animation();
}

static void stats_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_BTN_ANALOG_FILTER);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_START_MENU);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_SELECT_CONFIG);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 4;

    data_calculate_stats(0, 0xFFFFFFFF);
    stats_set_mode(STATS_PERIOD_WEEKLY);
}

static void stats_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    static int joystick_cooldown = 0;
    if (joystick_cooldown > 0) joystick_cooldown--;

    const SceCtrlData* pad_ptr = ui_get_pad();

    int analog_left = (pad_ptr->Lx < 64);
    int analog_right = (pad_ptr->Lx > 192);

    if ((pressed & PSP_CTRL_LEFT) || (analog_left && joystick_cooldown == 0)) {
        StatsPeriod next = (g_current_query.period + STATS_PERIOD_COUNT - 1) % STATS_PERIOD_COUNT;
        stats_set_mode(next);
        joystick_cooldown = 20; // ~333ms at 60fps
    } else if ((pressed & PSP_CTRL_RIGHT) || (analog_right && joystick_cooldown == 0)) {
        StatsPeriod next = (g_current_query.period + 1) % STATS_PERIOD_COUNT;
        stats_set_mode(next);
        joystick_cooldown = 20;
    }
}

static void stats_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    // 1. Header: Primary Title "Statistics"
    ui_draw_title_auto(i18n_get(MSG_MENU_STATS), safe_rect, &GD_IMG_ICON_STATS_32_PNG);

    // 2. Header: Filter Indicator (Right Aligned)
    const char *mode_str = "";
    if (g_current_query.period == STATS_PERIOD_WEEKLY) mode_str = i18n_get(MSG_STATS_MODE_WEEKLY);
    else if (g_current_query.period == STATS_PERIOD_MONTHLY) mode_str = i18n_get(MSG_STATS_MODE_MONTHLY);
    else if (g_current_query.period == STATS_PERIOD_YEARLY) mode_str = i18n_get(MSG_STATS_MODE_YEARLY);

    char filter_buf[64];
    snprintf(filter_buf, sizeof(filter_buf), "%s: %s", i18n_get(MSG_FILTER), mode_str);

    float filter_text_size = 1.0f;
    int filter_spacing = 8;
    int icon_size = 24; // slightly smaller for filter
    float text_w = font_get_width(filter_buf, filter_text_size);
    int total_filter_w = icon_size + filter_spacing + (int)text_w;

    int filter_x = safe_rect.x + safe_rect.w - total_filter_w;
    int filter_y = safe_rect.y + 8; // Exactly same baseline as main title

    // Perfect vertical alignment logic
    float filter_center_y = filter_y - (filter_text_size * 6.0f);
    int icon_y = (int)(filter_center_y - (icon_size / 2.0f));

    // Draw filter icon in white
    sceGuColor(COLOR_TEXT);
    texture_draw_resource(&GD_IMG_ICON_FILTER_32_PNG, filter_x, icon_y, icon_size, icon_size);

    // Draw filter text
    font_draw_string(filter_x + icon_size + filter_spacing, filter_y, filter_buf, COLOR_TEXT, filter_text_size);

    // 3. Main Content: Cached Graph Area
    ui_draw_stats_graph(&g_cached_graph_data, 240, 190, 360, 80);


    ui_draw_standard_hints();
}

Screen g_screen_stats = {
    stats_init,
    stats_update,
    stats_draw,
    NULL
};
