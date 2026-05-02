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
  * @file game_details.c
  * @brief Game details screen implementation.
  */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_text.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include "common/utils.h"
#include "common/models.h"
#include "common/db_schema.h"
#include "app/data/game_category.h"
#include "app/data/stats_calculator.h"
#include "app/audio/audio_manager.h"
#include "common/utils.h"
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>

static int g_game_idx = 0;
static Texture* g_game_icon = NULL;

static GameDetailStats s_stats;

static const char* s_helper_lines[8];
static PopupData s_helper_data;

// 0 = Summary, 1 = Weekly, 2 = Monthly, 3 = Last 12 Months, 4 = Yearly
static int s_details_page = 0;
static StatsQuery g_current_query = {STATS_PERIOD_WEEKLY, 0, 0};
static StatsGraphData g_cached_graph_data;
static u32 s_last_nav_ms = 0;

void game_details_set_idx(int idx) {
    g_game_idx = idx;
}


static void game_details_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_ANALOG_NAVIGATE);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_START_MENU);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_SELECT_CONFIG);
    s_helper_lines[5] = "";
    s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_DETAILS);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;

    if (g_game_icon) texture_free(g_game_icon);

    GameStats* games = data_get_games();
    GameStats* g = &games[g_game_idx];

    char icon_path[256];
    snprintf(icon_path, sizeof(icon_path), "%s" GDIARY_BASE_DIR "/" GDIARY_ICON_DIR "/%s.png",
             utils_get_device_prefix(), g->entry.game_id);
    g_game_icon = texture_load_png(icon_path);

    /* Delegate period calculations to data layer (uses PSP RTC correctly). */
    data_compute_game_details(g->entry.uid, &s_stats);

    s_details_page = 0;
    g_current_query.filter_uid = g->entry.uid;
    g_current_query.offset = 0;
}

static void game_details_update(u32 buttons, u32 pressed) {
    (void)buttons;
    (void)pressed;

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
        s_details_page--;
        if (s_details_page < 0) s_details_page = 4;
        audio_play_sfx(SFX_NAVIGATE);
        s_last_nav_ms = utils_get_time_ms();
        joystick_cooldown = 20;

        if (s_details_page > 0) {
            g_current_query.period = (StatsPeriod)(s_details_page - 1);
            stats_calc_query(data_get_sessions(), data_get_session_count(), g_current_query, &g_cached_graph_data);
            ui_reset_stats_graph_animation();
        }
    } else if ((pressed & PSP_CTRL_RIGHT) || (analog_right && joystick_cooldown == 0)) {
        s_details_page++;
        if (s_details_page > 4) s_details_page = 0;
        audio_play_sfx(SFX_NAVIGATE);
        s_last_nav_ms = utils_get_time_ms();
        joystick_cooldown = 20;

        if (s_details_page > 0) {
            g_current_query.period = (StatsPeriod)(s_details_page - 1);
            stats_calc_query(data_get_sessions(), data_get_session_count(), g_current_query, &g_cached_graph_data);
            ui_reset_stats_graph_animation();
        }
    }

    // inputs like Triangle, Start, Select are handled globally by screen_manager
}

static void draw_stat_row(Rect r, const char* label, const char* value) {
    ui_draw_text(label, r, COLOR_SUBTEXT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
    ui_draw_text(value, r, COLOR_TEXT, UI_FONT_SIZE_NORMAL, ALIGN_RIGHT);
}

static void game_details_draw(void) {
    renderer_clear(COLOR_BG);

    GameStats* games = data_get_games();
    GameStats* g = &games[g_game_idx];

    /* ----------------------------------------------------------------
     * SECTION 1 — Header card (icon + name + category)
     * Icon is drawn at its NATIVE size (no scaling).
     * Text column begins immediately after icon + 15px gap so different
     * icon widths are handled automatically.
     * Card height: 85px. Icon is vertically centered within that.
     * ---------------------------------------------------------------- */
    Rect header_card = {20, 8, 440, 85};
    ui_draw_card(header_card, COLOR_CARD, COLOR_BORDER);

    /* Icon — native size, vertically centered, 10px left margin inside card */
    sceGuColor(0xFFFFFFFF);
    int icon_w = 0, icon_h = 0;
    if (g_game_icon) {
        icon_w = g_game_icon->width;
        icon_h = g_game_icon->height;
        int oy = 8 + (85 - icon_h) / 2;  /* card inner height centers icons */
        texture_draw(g_game_icon, 30, oy, icon_w, icon_h);
    } else {
        const ImageResource* res = &GD_IMG_ICON_NOT_FOUND_PNG;
        icon_w = (int)res->width;
        icon_h = (int)res->height;
        int oy = 8 + (85 - icon_h) / 2;
        texture_draw_resource(res, 30, oy, icon_w, icon_h);
    }

    /* Text area: starts right after icon + 15px gap.
     * Card ends at x=460 (20+440); leave 10px right padding → text_max_x=450. */
    int tx = 30 + icon_w + 15;
    int tw_max = 450 - tx;
    if (tw_max < 50) tw_max = 50; /* safety floor */

    /* Game name — use auto-fit (scale then ellipsis) for long titles */
    ui_draw_game_name_auto_fit(g->entry.game_name, (Rect){tx, 14, tw_max, 22},
                               COLOR_ACCENT, UI_FONT_SIZE_TITLE_HUGE, ALIGN_LEFT);

    /* ID + Category sub-line */
    const char* cat_str = game_category_get_name(g->entry.category);
    char sub_buf[80];
    snprintf(sub_buf, sizeof(sub_buf), "%s  ·  %s", g->entry.game_id, cat_str);
    ui_draw_text(sub_buf, (Rect){tx, 42, tw_max, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);

    /* Sessions count */
    char vbuf[32];
    snprintf(vbuf, sizeof(vbuf), "%lu", (unsigned long)g->session_count);
    ui_draw_text(i18n_get(MSG_STATS_SESSIONS), (Rect){tx, 66, tw_max, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
    float label_w = font_get_width(i18n_get(MSG_STATS_SESSIONS), UI_FONT_SIZE_SMALL);
    ui_draw_text(vbuf, (Rect){tx + (int)label_w + 6, 66, tw_max, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);

    // Show different content based on selected page
    if (s_details_page == 0) {
        /* ----------------------------------------------------------------
         * SECTION 2 — History card (left)
         * Card: x=20, y=125, w=210, h=115
         * Rows: First Played / Last Played
         * ---------------------------------------------------------------- */
        Rect hist_card = {20, 125, 210, 115};
        ui_draw_card(hist_card, COLOR_CARD, COLOR_BORDER);

        char fplay[32], lplay[32];
        strncpy(fplay, i18n_get(MSG_STATS_NEVER), sizeof(fplay) - 1); fplay[sizeof(fplay)-1] = '\0';
        strncpy(lplay, i18n_get(MSG_STATS_NEVER), sizeof(lplay) - 1); lplay[sizeof(lplay)-1] = '\0';

        if (s_stats.first_played > 0) {
            time_t ts_first = (time_t)s_stats.first_played;
            struct tm tm_first = *localtime(&ts_first);
            strftime(fplay, sizeof(fplay), i18n_get(MSG_DATE_FORMAT), &tm_first);
        }
        if (s_stats.last_played > 0) {
            time_t ts_last = (time_t)s_stats.last_played;
            struct tm tm_last = *localtime(&ts_last);
            strftime(lplay, sizeof(lplay), i18n_get(MSG_DATE_FORMAT), &tm_last);
        }

        /* First Played row */
        draw_stat_row((Rect){30, 137, 190, 16}, i18n_get(MSG_DETAILS_FIRST_PLAYED), fplay);
        /* separator */
        renderer_draw_rect(30, 160, 190, 1, COLOR_BORDER);
        /* Last Played row */
        draw_stat_row((Rect){30, 166, 190, 16}, i18n_get(MSG_STATS_LAST_PLAYED), lplay);

        /* Total playtime — bold row at the bottom of history card */
        renderer_draw_rect(30, 189, 190, 1, COLOR_BORDER);
        char total_buf[32];
        ui_format_duration(g->total_playtime, total_buf, sizeof(total_buf));
        ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), (Rect){30, 196, 190, 18}, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
        ui_draw_text(total_buf, (Rect){30, 196, 190, 18}, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_RIGHT);

        /* ----------------------------------------------------------------
         * SECTION 3 — Playtime card (right)
         * Card: x=240, y=125, w=220, h=115
         * Rows: Week / Month / Year
         * ---------------------------------------------------------------- */
        Rect time_card = {240, 125, 220, 115};
        ui_draw_card(time_card, COLOR_CARD, COLOR_BORDER);

        /* Week */
        ui_format_duration(s_stats.playtime_week, vbuf, sizeof(vbuf));
        draw_stat_row((Rect){250, 137, 200, 16}, i18n_get(MSG_TOP_WEEK), vbuf);

        renderer_draw_rect(250, 158, 200, 1, COLOR_BORDER);

        /* Month */
        ui_format_duration(s_stats.playtime_month, vbuf, sizeof(vbuf));
        draw_stat_row((Rect){250, 164, 200, 16}, i18n_get(MSG_TOP_MONTH), vbuf);

        renderer_draw_rect(250, 185, 200, 1, COLOR_BORDER);

        /* Year */
        ui_format_duration(s_stats.playtime_year, vbuf, sizeof(vbuf));
        draw_stat_row((Rect){250, 191, 200, 16}, i18n_get(MSG_TOP_YEAR), vbuf);
    } else {
        // Draw graph pages
        ui_draw_stats_graph(&g_cached_graph_data, 240, 210, 420, 75);

        // Add filter name indicator overlay correctly aligned with the header card border
        const char *mode_str = "";
        if (g_current_query.period == STATS_PERIOD_WEEKLY) mode_str = i18n_get(MSG_STATS_MODE_WEEKLY);
        else if (g_current_query.period == STATS_PERIOD_MONTHLY) mode_str = i18n_get(MSG_STATS_MODE_MONTHLY);
        else if (g_current_query.period == STATS_PERIOD_LAST_12_MONTHS) mode_str = i18n_get(MSG_STATS_MODE_MONTHS);
        else if (g_current_query.period == STATS_PERIOD_YEARLY) mode_str = i18n_get(MSG_STATS_MODE_YEARLY);

        char filter_buf[64];
        snprintf(filter_buf, sizeof(filter_buf), "%s: %s", i18n_get(MSG_FILTER), mode_str);

        int filter_spacing = 8;
        int icon_size = 16;
        float text_w = font_get_width(filter_buf, UI_FONT_SIZE_SMALL);
        int total_filter_w = icon_size + filter_spacing + (int)text_w;

        int filter_x = 460 - total_filter_w;
        int filter_y = 105; // Just below the header card

        sceGuColor(COLOR_SUBTEXT);
        texture_draw_resource(&GD_IMG_ICON_FILTER_32_PNG, filter_x, filter_y - (icon_size/2), icon_size, icon_size);
        font_draw_string(filter_x + icon_size + filter_spacing, filter_y + 4, filter_buf, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL);
    }

    // Always show navigation indicators for context switching, vertically centered.
    ui_draw_nav_indicators(136, true, true, true, true, s_last_nav_ms, COLOR_ACCENT);

    ui_draw_standard_hints();
}

static void game_details_destroy(void) {
    if (g_game_icon) {
        texture_free(g_game_icon);
        g_game_icon = NULL;
    }
    s_details_page = 0;
}

Screen g_screen_game_details = {
    game_details_init,
    game_details_update,
    game_details_draw,
    game_details_destroy
};
