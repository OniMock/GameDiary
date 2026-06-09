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
 * @file sessions.c
 * @brief Sessions screen implementation showing all historical sessions sorted newest to oldest.
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_text.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/data/data_loader.h"
#include "app/audio/audio_manager.h"
#include "app/ui/ui_loading.h"
#include "common/utils.h"
#include "common/models.h"
#include "common/db_schema.h"
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define VISIBLE_ROWS 5
#define ROW_HEIGHT 30
#define ROW_GAP 4
#define START_Y 68
#define MAX_SESSIONS 4096

/* Module State */
static int s_selected_idx = 0;
static int s_scroll_offset = 0;

static bool s_confirm_delete = false;
static bool s_is_deleting = false;
static u32 s_delete_start_ms = 0;
static int s_delete_pending = 0;
static int s_delete_result = 0;

static int s_session_order[MAX_SESSIONS];
static int s_session_count = 0;

/* Helper Popup data */
static const char* s_helper_lines[8];
static PopupData s_helper_data;

/* Deletion Result Popup */
static const char* s_delete_result_lines[2];
static PopupData s_delete_result_popup;

/**
 * @brief Compare sessions by timestamp descending (newest first).
 */
static int compare_sessions(const void *a, const void *b) {
    int idx_a = *(const int *)a;
    int idx_b = *(const int *)b;
    SessionEntry *sessions = data_get_sessions();
    u32 ts_a = sessions[idx_a].timestamp;
    u32 ts_b = sessions[idx_b].timestamp;
    if (ts_a > ts_b) return -1;
    if (ts_a < ts_b) return 1;
    return 0;
}

/**
 * @brief Rebuilds the local session indices mapped to the data_loader global sessions array.
 */
static void rebuild_sessions_list(void) {
    u32 total = data_get_session_count();
    s_session_count = (total > MAX_SESSIONS) ? MAX_SESSIONS : total;
    
    for (int i = 0; i < s_session_count; i++) {
        s_session_order[i] = i;
    }
    
    if (s_session_count > 0) {
        qsort(s_session_order, s_session_count, sizeof(int), compare_sessions);
    }

    if (s_selected_idx >= s_session_count) {
        s_selected_idx = s_session_count - 1;
    }
    if (s_selected_idx < 0) {
        s_selected_idx = 0;
    }

    if (s_scroll_offset > s_selected_idx) {
        s_scroll_offset = s_selected_idx;
    }
    if (s_scroll_offset + VISIBLE_ROWS <= s_selected_idx) {
        s_scroll_offset = s_selected_idx - VISIBLE_ROWS + 1;
    }
    if (s_scroll_offset < 0) {
        s_scroll_offset = 0;
    }
}

/**
 * @brief Shows the result popup of the deletion operation.
 */
static void sessions_show_delete_result(int success) {
    s_delete_result_lines[0] = i18n_get(success ? MSG_SESSION_DELETE_OK : MSG_BACKUP_ERROR);
    s_delete_result_lines[1] = "";
    s_delete_result_popup.title = i18n_get(success ? MSG_SUCCESS : MSG_ERROR);
    s_delete_result_popup.title_color = success ? COLOR_SUCCESS : COLOR_DANGER;
    s_delete_result_popup.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_delete_result_popup.lines = s_delete_result_lines;
    s_delete_result_popup.line_count = 1;
    s_delete_result_popup.show_close_hint = true;
    popup_open(&s_delete_result_popup);
}

/**
 * @brief Initialize Sessions screen variables, help dialog and state.
 */
static void sessions_init(void) {
    s_confirm_delete = false;
    s_is_deleting = false;
    s_delete_pending = 0;

    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_TRIANGLE_DELETE_SESS);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_X_BOTTOM);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[5] = "";
    s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_SESSIONS);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;

    /* Ensure stats calculation is up-to-date and rebuild sorted sessions list */
    data_calculate_stats(0, 0xFFFFFFFF);
    rebuild_sessions_list();

    s_selected_idx = 0;
    s_scroll_offset = 0;
}

/**
 * @brief Handle input updates for scrolling list and delete action.
 */
static void sessions_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    if (s_is_deleting) {
        u32 elapsed = utils_get_time_ms() - s_delete_start_ms;

        if (s_delete_pending && elapsed > 16) {
            int idx = s_session_order[s_selected_idx];
            SessionEntry *s = &data_get_sessions()[idx];
            s_delete_result = data_delete_session(s->game_uid, s->timestamp);
            s_delete_pending = 0;
        }

        if (!s_delete_pending && elapsed >= 1000) {
            ui_loading_hide();
            s_is_deleting = false;
            s_confirm_delete = false;

            if (s_delete_result == 0) {
                /* Recalculate stats and refresh list */
                data_calculate_stats(0, 0xFFFFFFFF);
                game_list_rebuild_after_data_change();
                rebuild_sessions_list();
                sessions_show_delete_result(1);
            } else {
                sessions_show_delete_result(0);
            }
        }
        return;
    }

    if (s_confirm_delete) {
        if (pressed & PSP_CTRL_CROSS) {
            audio_play_sfx(SFX_CONFIRM);
            ui_loading_show(i18n_get(MSG_LOADING));
            s_delete_start_ms = utils_get_time_ms();
            s_is_deleting = true;
            s_delete_pending = 1;
        } else if (pressed & PSP_CTRL_CIRCLE) {
            audio_play_sfx(SFX_CANCEL);
            s_confirm_delete = false;
        }
        return;
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        audio_play_sfx(SFX_CANCEL);
        screen_manager_pop();
        return;
    }

    if (s_session_count == 0) return;

    if (pressed & PSP_CTRL_TRIANGLE) {
        audio_play_sfx(SFX_NAVIGATE);
        s_confirm_delete = true;
        return;
    }

    if (pressed & PSP_CTRL_SQUARE) {
        s_selected_idx = s_session_count - 1;
        if (s_session_count > VISIBLE_ROWS) {
            s_scroll_offset = s_session_count - VISIBLE_ROWS;
        } else {
            s_scroll_offset = 0;
        }
        audio_play_sfx(SFX_NAVIGATE);
        return;
    }

    const SceCtrlData* pad_ptr = ui_get_pad();

    /* List Item Navigation (Up / Down) */
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
            repeat_timer_y = 20;
        } else {
            repeat_timer_y--;
            if (repeat_timer_y <= 0) {
                move_y = current_dir_y;
                repeat_timer_y = 6;
            }
        }
    } else {
        repeat_timer_y = 0;
    }
    last_dir_y = current_dir_y;

    if (move_y != 0) {
        s_selected_idx += move_y;

        /* Wrap around selection boundaries */
        if (s_selected_idx < 0) {
            s_selected_idx = s_session_count - 1;
            if (s_session_count > VISIBLE_ROWS) {
                s_scroll_offset = s_session_count - VISIBLE_ROWS;
            } else {
                s_scroll_offset = 0;
            }
        } else if (s_selected_idx >= s_session_count) {
            s_selected_idx = 0;
            s_scroll_offset = 0;
        } else {
            /* Standard window scroll adjustment */
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
 * @brief Renders the confirmation card for session deletion.
 */
static void sessions_draw_confirm_delete(void) {
    Rect card_rect = {20, 80, 440, 150};
    const char *warn_text;
    char line1[128];
    char line2[128];
    char line3[128];
    const char *nl;
    const char *nl2;
    Rect title_rect;
    int sep_y;
    Rect text_rect_1;
    Rect text_rect_2;
    Rect text_rect_3;

    ui_draw_card(card_rect, COLOR_CARD, COLOR_BORDER);

    title_rect = (Rect){card_rect.x, card_rect.y + 10, card_rect.w, 30};
    ui_draw_text(i18n_get(MSG_WARNING), title_rect, COLOR_DANGER, UI_FONT_SIZE_TITLE_HUGE,
                 ALIGN_CENTER);

    sep_y = card_rect.y + 40;
    renderer_draw_rect(card_rect.x + 20, sep_y, card_rect.w - 40, 1, COLOR_BORDER);

    warn_text = i18n_get(MSG_SESSION_DELETE_WARN);
    line1[0] = line2[0] = line3[0] = '\0';

    nl = strchr(warn_text, '\n');
    if (nl) {
        int len = (int)(nl - warn_text);
        if (len >= (int)sizeof(line1)) {
            len = (int)sizeof(line1) - 1;
        }
        strncpy(line1, warn_text, (size_t)len);
        line1[len] = '\0';
        nl2 = strchr(nl + 1, '\n');
        if (nl2) {
            len = (int)(nl2 - (nl + 1));
            if (len >= (int)sizeof(line2)) {
                len = (int)sizeof(line2) - 1;
            }
            strncpy(line2, nl + 1, (size_t)len);
            line2[len] = '\0';
            snprintf(line3, sizeof(line3), "%s", nl2 + 1);
        } else {
            snprintf(line2, sizeof(line2), "%s", nl + 1);
        }
    } else {
        snprintf(line1, sizeof(line1), "%s", warn_text);
    }

    text_rect_1 = (Rect){card_rect.x, sep_y + 12, card_rect.w, 18};
    text_rect_2 = (Rect){card_rect.x, sep_y + 32, card_rect.w, 18};
    text_rect_3 = (Rect){card_rect.x, sep_y + 52, card_rect.w, 18};

    ui_draw_text(line1, text_rect_1, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_CENTER);
    if (line2[0] != '\0') {
        ui_draw_text(line2, text_rect_2, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_CENTER);
    }
    if (line3[0] != '\0') {
        ui_draw_text(line3, text_rect_3, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_CENTER);
    }

    if (!s_is_deleting) {
        ui_draw_hint_footer(i18n_get(MSG_HELP_BTN_X_CONFIRM), 20, COLOR_SUBTEXT);
        ui_draw_hint_footer(i18n_get(MSG_HELP_BTN_O_BACK), 160, COLOR_SUBTEXT);
    }
}

/**
 * @brief Renders the Sessions list layout, items and headers.
 */
static void sessions_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    /* Draw Header Title */
    ui_draw_title_auto(i18n_get(MSG_MENU_SESSIONS), safe_rect, &GD_IMG_ICON_MENU_32_PNG);

    if (s_confirm_delete) {
        sessions_draw_confirm_delete();
        return;
    }

    /* Empty state handling */
    if (s_session_count == 0) {
        Rect msg_rect = {20, 136, 440, 20};
        ui_draw_text(i18n_get(MSG_STATS_NO_ACTIVITY), msg_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TITLE_HUGE, ALIGN_CENTER);
        ui_draw_standard_hints();
        return;
    }

    SessionEntry *sessions = data_get_sessions();
    GameStats *games = data_get_games();
    u32 games_count = data_get_game_count();

    bool show_scroll = (s_session_count > VISIBLE_ROWS);
    int card_w = 440;
    int col_jogo_x = 28;
    int col_jogo_w = 110;
    int col_inicio_x = 145;
    int col_inicio_w = 115;
    int col_fim_x = 265;
    int col_fim_w = 115;
    int col_tempo_x = 385;
    int col_tempo_w = 65;

    /* Draw Column Headers at Y = 50 */
    Rect header_game_rect = {col_jogo_x, 50, col_jogo_w, 14};
    Rect header_start_rect = {col_inicio_x, 50, col_inicio_w, 14};
    Rect header_end_rect = {col_fim_x, 50, col_fim_w, 14};
    Rect header_time_rect = {col_tempo_x, 50, col_tempo_w, 14};

    ui_draw_text(i18n_get(MSG_HEADER_GAME), header_game_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    ui_draw_text(i18n_get(MSG_HEADER_START), header_start_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    ui_draw_text(i18n_get(MSG_HEADER_END), header_end_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    ui_draw_text(i18n_get(MSG_HEADER_DURATION), header_time_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_RIGHT);

    int render_count = (s_session_count - s_scroll_offset < VISIBLE_ROWS)
                       ? s_session_count - s_scroll_offset
                       : VISIBLE_ROWS;

    for (int i = 0; i < render_count; i++) {
        int idx = s_scroll_offset + i;
        int real_sess_idx = s_session_order[idx];
        const SessionEntry *s = &sessions[real_sess_idx];

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

        /* Lookup game name from game_uid */
        const char *game_name = "Unknown Game";
        for (u32 g_i = 0; g_i < games_count; g_i++) {
            if (games[g_i].entry.uid == s->game_uid) {
                game_name = games[g_i].entry.game_name;
                break;
            }
        }

        /* 1. Game Name — auto scroll/marquee when highlighted */
        Rect name_rect = {col_jogo_x, row_y + 8, col_jogo_w, 14};
        ui_draw_game_name_fixed(game_name, name_rect, name_col, UI_FONT_SIZE_TINY, ALIGN_LEFT, (idx == s_selected_idx));

        /* 2. Format Start -> End Date/Time string using localized date format */
        time_t start = (time_t)s->timestamp;
        time_t end = (time_t)(s->timestamp + s->duration);
        struct tm tm_start = *localtime(&start);
        struct tm tm_end = *localtime(&end);

        char format_str[64];
        snprintf(format_str, sizeof(format_str), "%s - %%H:%%M", i18n_get(MSG_DATE_FORMAT));

        char start_str[32];
        char end_str[32];
        strftime(start_str, sizeof(start_str), format_str, &tm_start);
        strftime(end_str, sizeof(end_str), format_str, &tm_end);

        Rect start_rect = {col_inicio_x, row_y + 8, col_inicio_w, 14};
        Rect end_rect = {col_fim_x, row_y + 8, col_fim_w, 14};
        ui_draw_text(start_str, start_rect, name_col, UI_FONT_SIZE_TINY, ALIGN_LEFT);
        ui_draw_text(end_str, end_rect, name_col, UI_FONT_SIZE_TINY, ALIGN_LEFT);

        /* 3. Formatted Duration String — right-aligned */
        char duration_str[32];
        ui_format_duration(s->duration, duration_str, sizeof(duration_str));

        Rect duration_rect = {col_tempo_x, row_y + 8, col_tempo_w, 14};
        ui_draw_text(duration_str, duration_rect, name_col, UI_FONT_SIZE_NORMAL, ALIGN_RIGHT);
    }

    /* Draw Scrollbar if list overflows */
    if (show_scroll) {
        int list_h = VISIBLE_ROWS * (ROW_HEIGHT + ROW_GAP) - ROW_GAP;
        Rect scroll_bg = {466, START_Y, 4, list_h};
        renderer_draw_rect(scroll_bg.x, scroll_bg.y, scroll_bg.w, scroll_bg.h, COLOR_BORDER);

        float handle_h = (float)VISIBLE_ROWS / s_session_count * list_h;
        float handle_y = (float)s_scroll_offset / s_session_count * list_h;
        renderer_draw_rect(scroll_bg.x, START_Y + (int)handle_y, scroll_bg.w, (int)handle_h, COLOR_ACCENT);
    }

    /* Controller hints */
    ui_draw_standard_hints();
}

/**
 * @brief Reset sessions module state when leaving.
 */
static void sessions_destroy(void) {
    s_selected_idx = 0;
    s_scroll_offset = 0;
    s_confirm_delete = false;
    s_is_deleting = false;
    s_delete_pending = 0;
}

/* Screen description structure */
Screen g_screen_sessions = {
    sessions_init,
    sessions_update,
    sessions_draw,
    sessions_destroy
};
