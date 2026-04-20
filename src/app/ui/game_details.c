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
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include "common/utils.h"
#include "common/models.h"
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>

static int g_game_idx = 0;
static Texture* g_game_icon = NULL;

static GameDetailStats s_stats;

void game_details_set_idx(int idx) {
    g_game_idx = idx;
}

static const char* get_game_category(u8 category) {
    switch (category) {
        case 0: return i18n_get(MSG_CAT_PSP);
        case 1: return i18n_get(MSG_CAT_PSX);
        case 2: return i18n_get(MSG_CAT_HOMEBREW);
        case 3: return i18n_get(MSG_CAT_PSP);  /* VSH — runs on PSP XMB */
        default: return i18n_get(MSG_CAT_HOMEBREW);
    }
}

static void game_details_init(void) {
    if (g_game_icon) texture_free(g_game_icon);

    GameStats* games = data_get_games();
    GameStats* g = &games[g_game_idx];

    char icon_path[256];
    snprintf(icon_path, sizeof(icon_path), "%s/PSP/COMMON/GameDiary/icons/%s.png",
             utils_get_device_prefix(), g->entry.game_id);
    g_game_icon = texture_load_png(icon_path);

    /* Delegate period calculations to data layer (uses PSP RTC correctly). */
    data_compute_game_details(g->entry.uid, &s_stats);
}

static void game_details_update(u32 buttons, u32 pressed) {
    (void)buttons;
    (void)pressed;
    // inputs like Triangle, Start, Select are handled globally by screen_manager
}

static void draw_stat_row(Rect r, const char* label, const char* value) {
    ui_draw_text(label, r, COLOR_SUBTEXT, 0.8f, ALIGN_LEFT);
    ui_draw_text(value, r, COLOR_TEXT, 0.8f, ALIGN_RIGHT);
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
     * Card height: 107px. Icon is vertically centered within that.
     * ---------------------------------------------------------------- */
    Rect header_card = {20, 10, 440, 107};
    ui_draw_card(header_card, COLOR_CARD, COLOR_BORDER);

    /* Icon — native size, vertically centered, 10px left margin inside card */
    sceGuColor(0xFFFFFFFF);
    int icon_w = 0, icon_h = 0;
    if (g_game_icon) {
        icon_w = g_game_icon->width;
        icon_h = g_game_icon->height;
        int oy = 10 + (87 - icon_h) / 2;  /* card inner height ≈ 87px */
        texture_draw(g_game_icon, 30, oy, icon_w, icon_h);
    } else {
        const ImageResource* res = &GD_IMG_ICON_NOT_FOUND_PNG;
        icon_w = (int)res->width;
        icon_h = (int)res->height;
        int oy = 10 + (87 - icon_h) / 2;
        texture_draw_resource(res, 30, oy, icon_w, icon_h);
    }

    /* Text area: starts right after icon + 15px gap.
     * Card ends at x=460 (20+440); leave 10px right padding → text_max_x=450. */
    int tx = 30 + icon_w + 15;
    int tw_max = 450 - tx;
    if (tw_max < 50) tw_max = 50; /* safety floor */

    /* Game name — use auto-fit (scale then ellipsis) for long titles */
    ui_draw_text_auto_fit(g->entry.game_name, (Rect){tx, 18, tw_max, 22},
                          COLOR_ACCENT, 1.0f, ALIGN_LEFT);

    /* ID + Category sub-line */
    const char* cat_str = get_game_category(g->entry.category);
    char sub_buf[80];
    snprintf(sub_buf, sizeof(sub_buf), "%s  ·  %s", g->entry.game_id, cat_str);
    ui_draw_text(sub_buf, (Rect){tx, 50, tw_max, 14}, COLOR_SUBTEXT, 0.72f, ALIGN_LEFT);

    /* Sessions count */
    char vbuf[32];
    snprintf(vbuf, sizeof(vbuf), "%lu", (unsigned long)g->session_count);
    ui_draw_text(i18n_get(MSG_STATS_SESSIONS), (Rect){tx, 80, tw_max, 14}, COLOR_SUBTEXT, 0.72f, ALIGN_LEFT);
    ui_draw_text(vbuf,                         (Rect){tx, 80, tw_max, 14}, COLOR_TEXT,    0.72f, ALIGN_RIGHT);

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
    ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), (Rect){30, 196, 190, 18}, COLOR_SUBTEXT, 0.75f, ALIGN_LEFT);
    ui_draw_text(total_buf, (Rect){30, 196, 190, 18}, COLOR_TEXT, 0.9f, ALIGN_RIGHT);

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

    ui_draw_standard_hints();
}

static void game_details_destroy(void) {
    if (g_game_icon) {
        texture_free(g_game_icon);
        g_game_icon = NULL;
    }
}

Screen g_screen_game_details = {
    game_details_init,
    game_details_update,
    game_details_draw,
    game_details_destroy
};
