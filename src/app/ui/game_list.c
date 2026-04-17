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
 * @file game_list.c
 * @brief Lateral icon carousel screen — replaces the old text list.
 *
 * Layout (480×272 px):
 *   y=20        Title bar ("Games" icon + label)
 *   y=45..130   Carousel zone
 *                 prev icon  80×45  at x=20,  y=68   (dimmed, 50% alpha)
 *                 cur  icon 144×80  at x=168, y=47   (full alpha)
 *                 next icon  80×45  at x=376, y=68   (dimmed, 50% alpha)
 *   y=140..148  Game name (centred under icon)
 *   y=152..162  Game ID   (centred, subdued)
 *   y=168..198  3 stat chips: Total Time | Sessions | Days Active
 *   y=205..250  Session bar graph (last 8 sessions, Steam-style)
 *   y=255       Control hints
 *
 * Architecture:
 *   Domain      → carousel_state.h  (selection, animation, icon cache)
 *   Presentation→ this file         (pure drawing, reads from domain)
 *   Data        → data_loader.h     (inalterado)
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/carousel_state.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include "app/data/stats_calculator.h"
#include "common/utils.h"
#include <pspctrl.h>
#include <pspgu.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Layout constants — all in screen pixels
 * ----------------------------------------------------------------------- */

/* Carousel icon geometry */
#define CUR_ICON_W   144
#define CUR_ICON_H    80
#define SIDE_ICON_W   80
#define SIDE_ICON_H   45

/* Horizontal icon positions (left edge of each icon) */
#define ICON_PREV_X   20
#define ICON_CUR_X   168
#define ICON_NEXT_X  376

/* Vertical positions */
#define ICON_CUR_Y  (CAROUSEL_CENTER_Y - CUR_ICON_H  / 2)  /* 50          */
#define ICON_SIDE_Y (CAROUSEL_CENTER_Y - SIDE_ICON_H / 2)  /* 67          */

/* Center Y of each icon */
#define CAROUSEL_CENTER_Y  80

/* Info text positions (moved up slightly) */
#define NAME_Y  135

/* Stat chips */
#define CHIP_Y       155
#define CHIP_H        30
#define CHIP_W       120
#define CHIP_GAP      15
/* 3 chips centred: total = 3*CHIP_W + 2*CHIP_GAP = 390; start = (480-390)/2 */
#define CHIP_START_X  45

/* Session bar graph */
#define GRAPH_BASELINE_Y  250
#define GRAPH_MAX_BAR_H    42

/* Tint colour for side icons (50% opaque white — modulates texture alpha) */
#define SIDE_ICON_TINT   0x80FFFFFFu

/* -----------------------------------------------------------------------
 * Module state
 * ----------------------------------------------------------------------- */

static CarouselState g_cs;
static int           g_prev_idx = -1; /* Tracks game changes for graph reset */
static u32           s_last_selected_uid = 0; /* Save selection on screen transitions */
static int           s_analog_held_x = 0;     /* Analog held state (-1 left, 1 right, 0 neutral) */

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/**
 * Draws one icon from the carousel at the given position.
 * Falls back to the embedded "icon not found" resource when the texture
 * is not cached (e.g. game has no icon0).
 *
 * @param game_idx  Logical game index (used for cache lookup).
 * @param x         Left edge of draw area.
 * @param y         Top edge of draw area.
 * @param w         Draw width.
 * @param h         Draw height.
 * @param tint      ARGB tint/alpha.  0xFFFFFFFF = full color, no tint.
 */
static void draw_carousel_icon(int inf_idx, int cx, int cy, float scale,
                                u32 tint) {
    int base_h = 80;
    int base_w = 144; /* Default if no texture found */

    Texture *tex = carousel_get_icon(&g_cs, inf_idx);
    if (tex) {
        if (tex->height > 0) {
            base_w = (tex->width * base_h) / tex->height;
        }
    }

    int w = (int)((float)base_w * scale);
    int h = (int)((float)base_h * scale);
    int x = cx - w / 2;
    int y = cy - h / 2;

    if (tex) {
        texture_draw_tinted(tex, x, y, w, h, tint);
    } else {
        /* Fallback: embedded placeholder. */
        sceGuColor(tint);
        texture_draw_resource(&GD_IMG_ICON_NOT_FOUND_PNG, x, y, w, h);
    }
}

static void draw_stats_block(const GameStats *g,
                             const SessionEntry *sessions, int sess_count) {
    char val_buf[64];
    char text_buf[128];

    int start_x = 30;
    int start_y = 160;
    int line_gap  = 30;
    int box_w = 200;
    int box_h = 24;

    /* Total Playtime */
    Rect box1 = {start_x, start_y, box_w, box_h};
    ui_draw_card(box1, COLOR_CARD, COLOR_BORDER);
    ui_format_duration(g->total_playtime, val_buf, sizeof(val_buf));
    snprintf(text_buf, sizeof(text_buf), "%s: %s", i18n_get(MSG_STATS_TOTAL_PLAYTIME), val_buf);
    ui_draw_text(text_buf, (Rect){start_x + 10, start_y + 7, box_w - 20, 10},
                 COLOR_TEXT, 0.8f, ALIGN_LEFT);

    /* Last Played Date */
    time_t last_time = 0;
    for (int i = 0; i < sess_count; i++) {
        if (sessions[i].game_uid == g->entry.uid && sessions[i].duration > 0) {
            if (sessions[i].timestamp > last_time) last_time = sessions[i].timestamp;
        }
    }
    char last_str[32];
    strncpy(last_str, i18n_get(MSG_STATS_NEVER), sizeof(last_str));
    if (last_time > 0) {
        struct tm ts_tm = *localtime(&last_time);
        strftime(last_str, sizeof(last_str), i18n_get(MSG_DATE_FORMAT), &ts_tm);
    }
    Rect box2 = {start_x, start_y + line_gap, box_w, box_h};
    ui_draw_card(box2, COLOR_CARD, COLOR_BORDER);
    snprintf(text_buf, sizeof(text_buf), "%s: %s", i18n_get(MSG_STATS_LAST_PLAYED), last_str);
    ui_draw_text(text_buf, (Rect){start_x + 10, start_y + line_gap + 7, box_w - 20, 10},
                 COLOR_TEXT, 0.8f, ALIGN_LEFT);

    /* Days Active */
    Rect box3 = {start_x, start_y + line_gap * 2, box_w, box_h};
    ui_draw_card(box3, COLOR_CARD, COLOR_BORDER);
    int days = carousel_count_days_active(sessions, sess_count, g->entry.uid);
    snprintf(text_buf, sizeof(text_buf), i18n_get(MSG_STATS_DAYS_ACTIVE), days);
    ui_draw_text(text_buf, (Rect){start_x + 10, start_y + line_gap * 2 + 7, box_w - 20, 10},
                 COLOR_TEXT, 0.8f, ALIGN_LEFT);
}

/* -----------------------------------------------------------------------
 * Screen callbacks
 * ----------------------------------------------------------------------- */

static void game_list_init(void) {
    /* Enable analog sampling for joystick navigation */
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    /* Calculate global stats and correctly sort by playtime */
    data_calculate_stats(0, 0xFFFFFFFF);
    stats_sort_by_total();

    u32 count = data_get_game_count();
    carousel_init(&g_cs, (int)count);
    g_prev_idx = -1;
    ui_reset_session_graph_animation();

    /* Restore selection if transitioning back from game_details */
    if (s_last_selected_uid != 0) {
        GameStats *games = data_get_games();
        for (u32 i = 0; i < count; i++) {
            if (games[i].entry.uid == s_last_selected_uid) {
                carousel_set_index(&g_cs, i);
                break;
            }
        }
    }
}

static void game_list_update(u32 buttons, u32 pressed) {
    u32 count = data_get_game_count();

    if (count == 0) {
        if (pressed & PSP_CTRL_CIRCLE)
            screen_manager_set(&g_screen_dashboard);
        return;
    }

    /* Process Analog Joystick */
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);
    u32 mapped_buttons = buttons;
    u32 mapped_pressed = pressed;

    if (pad.Lx < 60) {
        if (s_analog_held_x != -1) { mapped_pressed |= PSP_CTRL_LEFT; s_analog_held_x = -1; }
        mapped_buttons |= PSP_CTRL_LEFT;
    } else if (pad.Lx > 190) {
        if (s_analog_held_x != 1) { mapped_pressed |= PSP_CTRL_RIGHT; s_analog_held_x = 1; }
        mapped_buttons |= PSP_CTRL_RIGHT;
    } else {
        s_analog_held_x = 0;
    }

    carousel_update(&g_cs, mapped_buttons, mapped_pressed);

    /* Empty logic */
    ui_reset_session_graph_animation();
    g_prev_idx = g_cs.current_idx;
    /* Infinite wrapping logic for getting the active game */
    int wrap_idx = ((g_cs.current_idx % (int)count) + (int)count) % (int)count;

    if (wrap_idx != g_prev_idx) {
        ui_reset_session_graph_animation();
        g_prev_idx = wrap_idx;
    }

    /* Navigate to game details using wrapped index */
    if (mapped_pressed & PSP_CTRL_CROSS) {
        GameStats *games = data_get_games();
        s_last_selected_uid = games[wrap_idx].entry.uid;
        game_details_set_idx(wrap_idx);
        screen_manager_set(&g_screen_game_details);
    }

    if (mapped_pressed & PSP_CTRL_CIRCLE) {
        s_last_selected_uid = 0; // Clear on return to dashboard
        screen_manager_set(&g_screen_dashboard);
    }
}

static void game_list_draw(void) {
    renderer_clear(COLOR_BG);

    /* ----------------------------------------------------------------
     * Title bar
     * ---------------------------------------------------------------- */
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect   = rect_padding(screen_rect, 20);
    ui_draw_title(i18n_get(MSG_MENU_GAMES), safe_rect,
                  &GD_IMG_GAME_ICON_PNG, 24);

    /* ----------------------------------------------------------------
     * Empty state
     * ---------------------------------------------------------------- */
    u32 count = data_get_game_count();
    if (count == 0) {
        ui_draw_text(i18n_get(MSG_ERROR_NO_GAMES),
                     (Rect){0, 136, 480, 20}, COLOR_SUBTEXT, 1.0f,
                     ALIGN_CENTER);
        return;
    }

    /* ----------------------------------------------------------------
     * Carousel zone (Depth-sorted rendering)
     * ---------------------------------------------------------------- */
    typedef struct {
        int   inf_idx;
        float t;
        float abs_t;
    } CarouselItem;

    CarouselItem items[7];
    int item_count = 0;

    for (int i = -3; i <= 3; i++) {
        int inf_idx = g_cs.current_idx + i;
        float t = (float)i - g_cs.anim_offset;
        float abs_t = fabsf(t);
        if (abs_t > 2.5f) continue; /* Out of screen range */

        items[item_count].inf_idx = inf_idx;
        items[item_count].t = t;
        items[item_count].abs_t = abs_t;
        item_count++;
    }

    /* Sort items by depth (furthest drawn first, largest abs_t) */
    for (int i = 0; i < item_count - 1; i++) {
        for (int j = i + 1; j < item_count; j++) {
            if (items[i].abs_t < items[j].abs_t) {
                CarouselItem temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }
        }
    }

    int center_x = 240;

    for (int i = 0; i < item_count; ++i) {
        float t = items[i].t;
        float abs_t = items[i].abs_t;

        float scale = 1.0f;
        if (abs_t <= 1.0f) {
            scale = 1.0f - 0.25f * abs_t;       /* 1.0 to 0.75 */
        } else if (abs_t <= 2.0f) {
            scale = 0.75f - 0.25f * (abs_t - 1.0f); /* 0.75 to 0.50 */
        } else {
            scale = 0.50f - 0.25f * (abs_t - 2.0f); /* 0.50 to 0.25 */
        }

        float alpha_f = 1.0f;
        if (abs_t <= 1.0f) {
            alpha_f = 1.0f - 0.40f * abs_t;     /* 1.0 to 0.60 */
        } else if (abs_t <= 2.0f) {
            alpha_f = 0.60f - 0.60f * (abs_t - 1.0f); /* 0.60 to 0.00 */
        } else {
            alpha_f = 0.0f;
        }
        if (alpha_f < 0.0f) alpha_f = 0.0f;

        /* Distances from center: Overlapping effect, spaced out */
        float x_off;
        if (abs_t <= 1.0f) {
            x_off = 130.0f * abs_t;
        } else if (abs_t <= 2.0f) {
            x_off = 130.0f + 65.0f * (abs_t - 1.0f);
        } else {
            x_off = 195.0f + 45.0f * (abs_t - 2.0f);
        }

        int draw_x = center_x + (int)(t < 0 ? -x_off : x_off);

        if (alpha_f <= 0.01f) continue;
        u8 a = (u8)(alpha_f * 255.0f);
        u32 tint = ((u32)a << 24) | 0x00FFFFFFu;

        draw_carousel_icon(items[i].inf_idx, draw_x, CAROUSEL_CENTER_Y,
                           scale, tint);

        /* Accent border only for the perfectly centered target */
        if (abs_t <= 0.05f) {
            int w = (int)(144.0f * scale); /* Approximated base width for bar */
            renderer_draw_rect(draw_x - w/2, CAROUSEL_CENTER_Y + 42,
                               w, 2, COLOR_ACCENT);
        }
    }

    /* ----------------------------------------------------------------
     * Game info strip (name)
     * ---------------------------------------------------------------- */
    GameStats *games = data_get_games();
    int wrap_idx = ((g_cs.current_idx % (int)count) + (int)count) % (int)count;
    GameStats *g     = &games[wrap_idx];

    /* Clamp long names to avoid overflowing the info strip. */
    Rect name_rect = {60, NAME_Y, 360, 14};
    ui_draw_text(g->entry.game_name, name_rect, COLOR_TEXT, 0.9f,
                 ALIGN_CENTER);

    /* ----------------------------------------------------------------
     * Thin separator between info and stats
     * ---------------------------------------------------------------- */
    renderer_draw_rect(30, 150, 420, 1, COLOR_BORDER);

    /* Vertical line dividing Stats (Left) and Graph (Right) */
    renderer_draw_rect(240, 160, 1, 80, COLOR_BORDER);

    /* ----------------------------------------------------------------
     * Stats Vertical Block Area (Left Side)
     * ---------------------------------------------------------------- */
    SessionEntry *sessions   = data_get_sessions();
    u32           sess_count = data_get_session_count();
    draw_stats_block(g, sessions, (int)sess_count);

    /* ----------------------------------------------------------------
     * Session bar graph (Right Side, 3 bars max, dates shown)
     * ---------------------------------------------------------------- */
    ui_draw_session_bar_graph(sessions, (int)sess_count, g->entry.uid,
                              3,
                              360,          /* center_x of Graph is 360 */
                              240,          /* baseline_y */
                              55);          /* max_height */

    /* ----------------------------------------------------------------
     * Control hints
     * ---------------------------------------------------------------- */
    const char *back_label   = i18n_get(MSG_CTRL_BACK);
    const char *select_label = i18n_get(MSG_CTRL_SELECT);
    char hint_back[64], hint_sel[64];
    snprintf(hint_back, sizeof(hint_back), "|%s| %s",
             UI_SYM_CIRCLE_OPEN, back_label);
    snprintf(hint_sel,  sizeof(hint_sel),  "%s |X|", select_label);

    ui_draw_hint_footer(hint_back, 10, COLOR_SUBTEXT);
    float rw = font_get_width(hint_sel, 0.8f);
    ui_draw_hint_footer(hint_sel, 480 - 10 - (int)rw, COLOR_SUBTEXT);
}

static void game_list_destroy(void) {
    carousel_destroy(&g_cs);
}

/* -----------------------------------------------------------------------
 * Screen descriptor (replaces the old g_screen_game_list)
 * ----------------------------------------------------------------------- */
Screen g_screen_game_list = {
    game_list_init,
    game_list_update,
    game_list_draw,
    game_list_destroy
};
