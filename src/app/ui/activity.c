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
 * @file activity.c
 * @brief Activity screen implementation.
 */

#include "app/data/data_loader.h"
#include "app/data/stats_calculator.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/render/font.h"
#include "app/data/game_category.h"
#include <pspctrl.h>
#include <stdio.h>

static const char* s_helper_lines[3];
static PopupData s_helper_data;

static void activity_init(void) {
  s_helper_lines[0] = i18n_get(MSG_HELP_BTN_O_BACK);
  s_helper_lines[1] = i18n_get(MSG_HELP_BTN_START_MENU);
  s_helper_lines[2] = i18n_get(MSG_HELP_BTN_SELECT_CONFIG);

  s_helper_data.title = i18n_get(MSG_HELP_TITLE);
  s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
  s_helper_data.lines = s_helper_lines;
  s_helper_data.line_count = 3;

  // Standard all-time stats
  data_calculate_stats(0, 0xFFFFFFFF);
}

static void activity_update(u32 buttons, u32 pressed) {
  (void)buttons;

  if (pressed & PSP_CTRL_LTRIGGER) {
      popup_open(&s_helper_data);
      return;
  }

  // Handled by global inputs in screen_manager.c
}


static void activity_draw(void) {
  renderer_clear(COLOR_BG);

  Rect screen_rect = {0, 0, 480, 272};
  Rect safe_rect = rect_padding(screen_rect, 20);

  ui_draw_title_auto(i18n_get(MSG_MENU_ACTIVITY), safe_rect, &GD_IMG_ICON_ACTIVITY_32_PNG);

  GameStats *games = data_get_games();
  u32 total_games = data_get_game_count();

  if (total_games == 0) {
      Rect msg_rect = {20, 100, 440, 40};
      ui_draw_text(i18n_get(MSG_ERROR_NO_GAMES), msg_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TITLE_HUGE, ALIGN_CENTER);
      ui_draw_standard_hints();
      return;
  }

  // 1. Find the top 5 most recently played games
  int recent_indices[5] = {-1, -1, -1, -1, -1};
  u32 recent_counts = 0;

  // We'll do a simple selection sort for the top 5
  u32 last_ts_found[5] = {0};

  for (u32 i = 0; i < total_games; i++) {
      u32 ts = games[i].last_played_ts;
      if (ts == 0) continue;

      for (int j = 0; j < 5; j++) {
          if (ts > last_ts_found[j]) {
              // Shift others down
              for (int k = 4; k > j; k--) {
                  last_ts_found[k] = last_ts_found[k - 1];
                  recent_indices[k] = recent_indices[k - 1];
              }
              last_ts_found[j] = ts;
              recent_indices[j] = i;
              if (recent_counts < 5) recent_counts++;
              break;
          }
      }
  }

  // Global totals
  u32 global_playtime = 0;
  u32 global_sessions = 0;
  for (u32 i = 0; i < total_games; i++) {
      global_playtime += games[i].total_playtime;
      global_sessions += games[i].session_count;
  }

  // 2. LAYOUT - TOP SECTION (Summary & Focus)
  int top_y = 50;
  int card_h = 80;

  // Left: Focus Card (Last Game)
  if (recent_indices[0] != -1) {
      GameStats *last_g = &games[recent_indices[0]];
      Rect focus_rect = {20, top_y, 250, card_h};
      ui_draw_card(focus_rect, COLOR_CARD, COLOR_BORDER);
      Rect focus_padded = rect_padding(focus_rect, 10);

      // Label "Último Jogo"
      ui_draw_text(i18n_get(MSG_STATS_LAST_PLAYED), (Rect){focus_padded.x, focus_padded.y, focus_padded.w, 15}, COLOR_ACCENT, UI_FONT_SIZE_TINY, ALIGN_LEFT);

      // Title
      ui_draw_text(last_g->entry.game_name, (Rect){focus_padded.x, focus_padded.y + 18, focus_padded.w, 20}, COLOR_TEXT, UI_FONT_SIZE_TITLE_LIST, ALIGN_LEFT);

      // Detailed Playtime
      char dur_buf[32];
      ui_format_duration(last_g->total_playtime, dur_buf, sizeof(dur_buf));
      char final_dur[64];
      snprintf(final_dur, sizeof(final_dur), "%s: %s", i18n_get(MSG_STATS_TOTAL_PLAYTIME), dur_buf);
      ui_draw_text(final_dur, (Rect){focus_padded.x, focus_padded.y + 40, focus_padded.w, 15}, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);

      // Date
      time_t lplayed = (time_t)last_g->last_played_ts;
      struct tm *lt = localtime(&lplayed);
      char date_buf[32];
      strftime(date_buf, sizeof(date_buf), i18n_get(MSG_DATE_FORMAT), lt);
      ui_draw_text(date_buf, (Rect){focus_padded.x, focus_padded.y + 55, focus_padded.w, 15}, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
  }

  // Right: Global Stats
  int right_x = 280;
  int sub_card_h = (card_h - 8) / 2;

  // Total Playtime Global
  Rect glob_play_rect = {right_x, top_y, 180, sub_card_h};
  ui_draw_card(glob_play_rect, COLOR_CARD, COLOR_BORDER);
  char glob_time[32];
  ui_format_duration(global_playtime, glob_time, sizeof(glob_time));
  ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), (Rect){right_x + 8, top_y + 4, 160, 12}, COLOR_SUBTEXT, UI_FONT_SIZE_MICRO, ALIGN_LEFT);
  ui_draw_text(glob_time, (Rect){right_x + 8, top_y + 16, 160, 15}, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_LEFT);

  // Total Sessions Global
  Rect glob_sess_rect = {right_x, top_y + sub_card_h + 8, 180, sub_card_h};
  ui_draw_card(glob_sess_rect, COLOR_CARD, COLOR_BORDER);
  char sess_buf[32];
  snprintf(sess_buf, sizeof(sess_buf), "%lu", (unsigned long)global_sessions);
  ui_draw_text(i18n_get(MSG_STATS_SESSIONS), (Rect){right_x + 8, top_y + sub_card_h + 12, 160, 12}, COLOR_SUBTEXT, UI_FONT_SIZE_MICRO, ALIGN_LEFT);
  ui_draw_text(sess_buf, (Rect){right_x + 8, top_y + sub_card_h + 24, 160, 15}, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_LEFT);

  // 3. LAYOUT - BOTTOM SECTION (Recent Activity Card)
  int list_y = top_y + card_h + 8;
  Rect history_card_rect = {20, list_y, 440, 105};
  ui_draw_card(history_card_rect, COLOR_CARD, COLOR_BORDER);

  ui_draw_text(i18n_get(MSG_MENU_ACTIVITY), (Rect){30, list_y + 8, 420, 15}, COLOR_ACCENT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
  renderer_draw_rect(30, list_y + 24, 420, 1, COLOR_BORDER & 0x66FFFFFF);

  for (int i = 1; i < (int)recent_counts; i++) { // Skip the first one which is prominent above
      int idx = recent_indices[i];
      if (idx == -1) break;

      int row_y = list_y + 28 + (i - 1) * 18;
      GameStats *g = &games[idx];

      // Category Badge
      char cat_tag[32] = {0};
      snprintf(cat_tag, sizeof(cat_tag), "[%s]", game_category_get_name(g->entry.category));

      float tag_w = font_get_width(cat_tag, UI_FONT_SIZE_MICRO);
      ui_draw_text(cat_tag, (Rect){30, row_y, (int)tag_w, 20}, COLOR_SUBTEXT, UI_FONT_SIZE_MICRO, ALIGN_LEFT);

      // Game Name
      ui_draw_text(g->entry.game_name, (Rect){30 + (int)tag_w + 5, row_y, 240, 20}, COLOR_TEXT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);

      // Last Played Date (Full)
      time_t item_ts = (time_t)g->last_played_ts;
      struct tm *lt = localtime(&item_ts);
      char date_buf[32];
      strftime(date_buf, sizeof(date_buf), i18n_get(MSG_DATE_FORMAT), lt);
      ui_draw_text(date_buf, (Rect){330, row_y, 120, 20}, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL, ALIGN_RIGHT);
  }

  ui_draw_standard_hints();
}

Screen g_screen_activity = {activity_init, activity_update, activity_draw, NULL};
