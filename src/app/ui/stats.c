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
 * @file stats.c
 * @brief Stats screen implementation.
 */

#include "app/data/data_loader.h"
#include "app/data/stats_calculator.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "common/utils.h"
#include <pspctrl.h>
#include <stdio.h>


static void stats_init(void) {
  // Standard all-time stats
  data_calculate_stats(0, 0xFFFFFFFF);
}

static void stats_update(u32 buttons, u32 pressed) {
  (void)buttons;
  (void)pressed;
  // Handled by global inputs in screen_manager.c
}


static void stats_draw(void) {
  renderer_clear(COLOR_BG);

  Rect screen_rect = {0, 0, 480, 272};
  Rect safe_rect = rect_padding(screen_rect, 20);

  ui_draw_title_auto(i18n_get(MSG_MENU_ACTIVITY), safe_rect, &GD_IMG_ICON_ACTIVITY_32_PNG);

  u32 total_play = 0;
  u32 total_sessions = 0;
  GameStats *games = data_get_games();
  u32 count = data_get_game_count();

  for (u32 i = 0; i < count; i++) {
    total_play += games[i].total_playtime;
    total_sessions += games[i].session_count;
  }

  // Stats Grid
  Rect grid_area = {20, 80, 440, 160};
  Rect play_rect = rect_column(grid_area, 0, 2, 10);
  Rect session_rect = rect_column(grid_area, 1, 2, 10);

  // Playtime Card
  ui_draw_card(play_rect, COLOR_CARD, COLOR_BORDER);
  Rect p_cont = rect_padding(play_rect, 10);
  ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), rect_column(p_cont, 0, 2, 0),
               COLOR_SUBTEXT, 0.7f, ALIGN_LEFT);
  char time_str[32];
  ui_format_duration(total_play, time_str, sizeof(time_str));
  ui_draw_text(time_str, rect_column(p_cont, 1, 2, 0), COLOR_TEXT, 1.2f,
               ALIGN_LEFT);

  // Sessions Card
  ui_draw_card(session_rect, COLOR_CARD, COLOR_BORDER);
  Rect s_cont = rect_padding(session_rect, 10);
  ui_draw_text(i18n_get(MSG_STATS_SESSIONS), rect_column(s_cont, 0, 2, 0),
               COLOR_SUBTEXT, 0.7f, ALIGN_LEFT);
  char sess_str[16];
  snprintf(sess_str, sizeof(sess_str), "%lu", (unsigned long)total_sessions);
  ui_draw_text(sess_str, rect_column(s_cont, 1, 2, 0), COLOR_TEXT, 1.2f,
               ALIGN_LEFT);
  ui_draw_standard_hints();
}

Screen g_screen_stats = {stats_init, stats_update, stats_draw, NULL};
