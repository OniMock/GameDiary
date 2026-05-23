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
 * @file leaderboard.c
 * @brief Leaderboard screen implementation showing ranked games by playtime.
 *
 * Layout (480×272 px):
 *   y=20        Title bar ("Leaderboard" icon + label)
 *   y=50..72    Horizontal Category Tabs Selector
 *   y=80..246   5 ranked game rows:
 *                 - Game name (left-aligned, auto-fitted)
 *                 - Horizontal bar (centered, scaled relative to top game)
 *                 - Playtime string (right-aligned)
 *   y=267       Control hints
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "common/utils.h"
#include "app/ui/ui_text.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include "app/data/stats_calculator.h"
#include "app/data/game_category.h"
#include "app/audio/audio_manager.h"
#include <pspctrl.h>
#include <pspgu.h>
#include <stdio.h> // for snprintf to format ranking numbers
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define VISIBLE_ROWS 5
#define ROW_HEIGHT 30
#define ROW_GAP 4
#define START_Y 80
#define MAX_BAR_WIDTH 160


/*
 * Maximum number of categories: FILTER_ALL + all CAT_* values (CAT_UNKNOWN+1).
 * CAT_UNKNOWN = 4, so at most 1 + 5 = 6 slots needed.
 */
#define MAX_CATEGORY_SLOTS (CAT_UNKNOWN + 2)

/*
 * Dynamic category list built at init time using only categories that
 * have at least one game in the database. FILTER_ALL is always first.
 */
static u8  s_dynamic_cats[MAX_CATEGORY_SLOTS];
static int s_category_count = 0;

/**
 * @brief Get the short stylized name for a category to prevent screen overflow.
 */
static const char* get_category_short_name(u8 cat) {
    switch (cat) {
        case FILTER_ALL:   return i18n_get(MSG_TOP_ALL);     /* "All"      */
        case CAT_PSP:      return "PSP";
        case CAT_PS1:      return "PSX";
        case CAT_HOMEBREW: return "Homebrew";
        default:           return i18n_get(MSG_CAT_UNKNOWN); /* "Unknown"  */
    }
}

/* Module State */
static int s_category_idx = 0;
static u8  s_current_filter = FILTER_ALL;

static int s_filtered_indices[512];
static int s_filtered_count = 0;

static int s_selected_idx = 0;
static int s_scroll_offset = 0;

static u32 s_last_nav_ms = 0;

/* Helper Popup data */
static const char* s_helper_lines[8];
static PopupData s_helper_data;

/**
 * @brief Recalculate list of games matching the selected category filter.
 * Keeps the sorted order from data_loader.
 */
static void update_filtered_list(void) {
    u32 total = data_get_game_count();
    GameStats *all_games = data_get_games();
    s_filtered_count = 0;

    for (u32 i = 0; i < total; i++) {
        if (s_current_filter == FILTER_ALL ||
            game_category_normalize(all_games[i].entry.category) == s_current_filter) {
            if (s_filtered_count < 512) {
                s_filtered_indices[s_filtered_count++] = i;
            }
        }
    }
}

/**
 * @brief Initialize Leaderboard screen variables, assets and state.
 */
static void leaderboard_init(void) {
    /* Setup help dialog strings */
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_ANALOG_FILTER);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_TRIANGLE_TOP);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_X_BOTTOM);
    s_helper_lines[5] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[6] = "";
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_LEADERBOARD);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;

    /* Ensure joystick is sampled properly */
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    /* Recalculate stats and sort all games from most to least played */
    data_calculate_stats(0, 0xFFFFFFFF);
    stats_sort_by_total();

    /* Build the dynamic category list: FILTER_ALL first, then only
     * categories that have at least one game in the database. */
    s_dynamic_cats[0] = FILTER_ALL;
    s_category_count  = 1 + game_category_get_available(&s_dynamic_cats[1]);

    /* Reset selection to the top */
    s_category_idx = 0;
    s_current_filter = s_dynamic_cats[s_category_idx];
    s_selected_idx = 0;
    s_scroll_offset = 0;

    update_filtered_list();
}

/**
 * @brief Handle input updates for navigation and category switching.
 */
static void leaderboard_update(u32 buttons, u32 pressed) {
    /* L-Trigger opens helper modal */
    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    const SceCtrlData* pad_ptr = ui_get_pad();

    /* 1. Category Switch Handling (Left / Right) */
    static int repeat_timer_x = 0;
    static int last_dir_x = 0;

    int current_dir_x = 0;
    float ax = (pad_ptr->Lx - 128.0f) / 128.0f;
    if ((buttons & PSP_CTRL_LEFT) || (ax < -0.70f)) current_dir_x = -1;
    else if ((buttons & PSP_CTRL_RIGHT) || (ax > 0.70f)) current_dir_x = 1;

    int move_x = 0;
    if (current_dir_x != 0) {
        if (current_dir_x != last_dir_x) {
            move_x = current_dir_x;
            repeat_timer_x = 25; /* Delay before initial repeat (approx 400ms) */
        } else {
            repeat_timer_x--;
            if (repeat_timer_x <= 0) {
                move_x = current_dir_x;
                repeat_timer_x = 15; /* Repeat speed (approx 250ms) */
            }
        }
    } else {
        repeat_timer_x = 0;
    }
    last_dir_x = current_dir_x;

    if (move_x != 0) {
        s_category_idx = (s_category_idx + s_category_count + move_x) % s_category_count;
        s_current_filter = s_dynamic_cats[s_category_idx];

        update_filtered_list();

        s_selected_idx = 0;
        s_scroll_offset = 0;

        audio_play_sfx(SFX_NAVIGATE);
        s_last_nav_ms = utils_get_time_ms();
    }

    /* Skip list navigation if no games are found under current filter */
    if (s_filtered_count == 0) return;

    /* 2. Jump to Top (Triangle) and Bottom (Square) */
    if (pressed & PSP_CTRL_TRIANGLE) {
        s_selected_idx = 0;
        s_scroll_offset = 0;
        audio_play_sfx(SFX_NAVIGATE);
        return;
    }

    if (pressed & PSP_CTRL_SQUARE) {
        s_selected_idx = s_filtered_count - 1;

        /* Adjust scroll offset to keep selected item at the bottom of the visible screen */
        if (s_filtered_count > VISIBLE_ROWS) {
            s_scroll_offset = s_filtered_count - VISIBLE_ROWS;
        } else {
            s_scroll_offset = 0;
        }
        audio_play_sfx(SFX_NAVIGATE);
        return;
    }

    /* 3. List Item Navigation (Up / Down) */
    static int repeat_timer_y = 0;
    static int last_dir_y = 0;

    int current_dir_y = 0;
    float ay = (pad_ptr->Ly - 128.0f) / 128.0f;
    if ((buttons & PSP_CTRL_UP) || (ay < -0.70f)) current_dir_y = -1;
    else if ((buttons & PSP_CTRL_DOWN) || (ay > 0.70f)) current_dir_y = 1;

    int move_y = 0;
    if (current_dir_y != 0) {
        if (current_dir_y != last_dir_y) {
            move_y = current_dir_y;
            repeat_timer_y = 20; /* Delay before initial repeat (approx 330ms) */
        } else {
            repeat_timer_y--;
            if (repeat_timer_y <= 0) {
                move_y = current_dir_y;
                repeat_timer_y = 6; /* Fast scrolling repeat speed (approx 100ms) */
            }
        }
    } else {
        repeat_timer_y = 0;
    }
    last_dir_y = current_dir_y;

    if (move_y != 0) {
        s_selected_idx += move_y;

        /* Standard wrapping logic */
        if (s_selected_idx < 0) {
            s_selected_idx = s_filtered_count - 1;
            if (s_filtered_count > VISIBLE_ROWS) {
                s_scroll_offset = s_filtered_count - VISIBLE_ROWS;
            } else {
                s_scroll_offset = 0;
            }
        } else if (s_selected_idx >= s_filtered_count) {
            s_selected_idx = 0;
            s_scroll_offset = 0;
        } else {
            /* Standard scrolling window adjustment */
            if (s_selected_idx < s_scroll_offset) {
                s_scroll_offset = s_selected_idx;
            } else if (s_selected_idx >= s_scroll_offset + VISIBLE_ROWS) {
                s_scroll_offset = s_selected_idx - (VISIBLE_ROWS - 1);
            }
        }

        audio_play_sfx(SFX_NAVIGATE);
    }
}

/**
 * @brief Render the Leaderboard title, categories, game rows, and navigation hints.
 */
static void leaderboard_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    /* 1. Title bar */
    ui_draw_title(i18n_get(MSG_MENU_LEADERBOARD), safe_rect,
                  &GD_IMG_ICON_LEADERBOARD_32_PNG, UI_ICON_SIZE_TITLE);

    /* 2. Center-Aligned Sliding Category Selector */
    int tab_y = 50;

    /* Previous Category (Left) */
    int prev_idx = (s_category_idx - 1 + s_category_count) % s_category_count;
    const char *prev_name = get_category_short_name(s_dynamic_cats[prev_idx]);
    Rect prev_rect = { 90 - 55, tab_y, 110, 22 };
    ui_draw_text(prev_name, prev_rect, UI_COLOR_ALPHA(COLOR_SUBTEXT, 80), UI_FONT_SIZE_MEDIUM, ALIGN_CENTER); // increased opacity and size for better readability

    /* Current Category (Center, Highlighted) */
    const char *curr_name = get_category_short_name(s_dynamic_cats[s_category_idx]);
    Rect curr_rect = { 240 - 70, tab_y, 140, 22 };
    ui_draw_card(curr_rect, COLOR_CARD, COLOR_ACCENT);
    ui_draw_text(curr_name, curr_rect, COLOR_ACCENT, UI_FONT_SIZE_SMALL, ALIGN_CENTER);

    /* Next Category (Right) */
    int next_idx = (s_category_idx + 1) % s_category_count;
    const char *next_name = get_category_short_name(s_dynamic_cats[next_idx]);
    Rect next_rect = { 390 - 55, tab_y, 110, 22 };
    ui_draw_text(next_name, next_rect, UI_COLOR_ALPHA(COLOR_SUBTEXT, 80), UI_FONT_SIZE_MEDIUM, ALIGN_CENTER); // increased opacity and size for better readability

    /* 3. Empty State Handling */
    if (s_filtered_count == 0) {
        Rect msg_rect = {20, 136, 440, 20};
        ui_draw_text(i18n_get(MSG_ERROR_NO_GAMES), msg_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TITLE_HUGE, ALIGN_CENTER);
        ui_draw_standard_hints();
        return;
    }

    /* 4. Render visible rows */
    GameStats *games = data_get_games();

    /* Playtime of top game in current filtered category (index 0 of sorted filtered indices) */
    u32 top_playtime = games[s_filtered_indices[0]].total_playtime;

    bool show_scroll = (s_filtered_count > VISIBLE_ROWS);
    int card_w = 440;
    int bar_x  = 200;  /* bar starts here — name column must not cross this */
    int bar_w  = 160;
    int time_x = 370;
    int time_w = 80;

    /* Rank number column: left-aligned near the card edge */
    int rank_x = 28;
    int rank_w = (s_filtered_count >= 100) ? 24 : (s_filtered_count >= 10) ? 18 : 12;

    /* Game name column: fills from rank end to bar start, leaving a clean gap before the bar */
    int name_x = rank_x + rank_w;
    int name_w = bar_x - name_x - 8; /* 8px gap before the progress bar */

    int render_count = (s_filtered_count - s_scroll_offset < VISIBLE_ROWS)
                       ? s_filtered_count - s_scroll_offset
                       : VISIBLE_ROWS;

    for (int i = 0; i < render_count; i++) {
        int idx = s_scroll_offset + i;
        int real_game_idx = s_filtered_indices[idx];
        GameStats *g = &games[real_game_idx];

        int row_y = START_Y + i * (ROW_HEIGHT + ROW_GAP);
        Rect r_rect = {20, row_y, card_w, ROW_HEIGHT};

        /* Color styling depending on selection state */
        u32 bg_col = COLOR_CARD;
        u32 border_col = COLOR_BORDER;
        u32 name_col = COLOR_TEXT;

        if (idx == s_selected_idx) {
            bg_col = COLOR_HIGHLIGHT;
            border_col = COLOR_ACCENT;
            name_col = COLOR_ACCENT;
        }

        /* Draw row container */
        ui_draw_card(r_rect, bg_col, border_col);

        /* 4.1a. Rank number — fixed size, left-aligned for a clean margin */
        char rank_str[16];
        snprintf(rank_str, sizeof(rank_str), "%d.", idx + 1);
        Rect rank_rect = {rank_x, row_y + 8, rank_w, 14};
        ui_draw_text(rank_str, rank_rect, name_col, UI_FONT_SIZE_TINY, ALIGN_LEFT);

        /* 4.1b. Game name — fixed size, ellipsis only when it overflows */
        Rect name_rect = {name_x, row_y + 8, name_w, 14};
        ui_draw_game_name_fixed(g->entry.game_name, name_rect, name_col, UI_FONT_SIZE_TINY, ALIGN_LEFT);

        /* 4.2. Playtime Horizontal Bar (Middle) */
        int bar_y = row_y + 11;
        int bar_h = 8;

        /* Draw empty bar track */
        renderer_draw_rect(bar_x, bar_y, bar_w, bar_h, UI_COLOR_ALPHA(COLOR_BORDER, 60));

        /* Draw filled progress bar relative to highest playtime game */
        int fill_w = 0;
        if (top_playtime > 0) {
            fill_w = (int)(((u64)g->total_playtime * bar_w) / top_playtime);
        }
        if (fill_w > 0) {
            renderer_draw_rect(bar_x, bar_y, fill_w, bar_h, COLOR_ACCENT);
        }

        /* 4.3. Formatted Playtime String (Right) */
        char time_str[32];
        ui_format_duration(g->total_playtime, time_str, sizeof(time_str));

        Rect time_rect = {time_x, row_y + 8, time_w, 14};
        ui_draw_text(time_str, time_rect, name_col, UI_FONT_SIZE_NORMAL, ALIGN_RIGHT);
    }

    /* 5. Draw Scrollbar if list overflows */
    if (show_scroll) {
        int list_h = VISIBLE_ROWS * (ROW_HEIGHT + ROW_GAP) - ROW_GAP;
        Rect scroll_bg = {466, START_Y, 4, list_h};
        renderer_draw_rect(scroll_bg.x, scroll_bg.y, scroll_bg.w, scroll_bg.h, COLOR_BORDER);

        float handle_h = (float)VISIBLE_ROWS / s_filtered_count * list_h;
        float handle_y = (float)s_scroll_offset / s_filtered_count * list_h;
        renderer_draw_rect(scroll_bg.x, START_Y + (int)handle_y, scroll_bg.w, (int)handle_h, COLOR_ACCENT);
    }

    /* 6. Draw Dynamic Tab Switching arrows on Left/Right edges of screen */
    ui_draw_nav_indicators(tab_y + 11, true, true, true, true, s_last_nav_ms, COLOR_ACCENT);

    /* 7. Standardized controller hints */
    ui_draw_standard_hints();
}

/* Screen description structure */
Screen g_screen_leaderboard = {
    leaderboard_init,
    leaderboard_update,
    leaderboard_draw,
    NULL
};
