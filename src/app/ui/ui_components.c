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
 * @file ui_components.c
 * @brief UI components implementation.
 */

#include "app/ui/ui_components.h"
#include "app/ui/ui_text.h"
#include "app/i18n/i18n.h"
#include "app/render/font.h"
#include "app/render/renderer.h"
#include "app/render/texture.h"
#include "common/utils.h"
#include <pspgu.h>
#include <psprtc.h>
#include <psptypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>

static float g_graph_anim_h[MAX_GRAPH_COLS] = {0.0f};
static float g_graph_anim_x[MAX_GRAPH_COLS] = {0.0f};
static float g_graph_anim_a[MAX_GRAPH_COLS] = {0.0f};

void ui_draw_card(Rect r, u32 bg_color, u32 border_color) {
  // Draw thin border (1px)
  renderer_draw_rect(r.x - 1, r.y - 1, r.w + 2, r.h + 2, border_color);
  // Draw main body
  renderer_draw_rect(r.x, r.y, r.w, r.h, bg_color);
}

void ui_draw_text(const char *text, Rect r, u32 color, float size,
                  UIAlign align) {
  if (!text)
    return;

  // The 0.35f ratio corresponds to the baseline offset (6px for a 17px font)
  float y_pos = floorf(r.y + (r.h / 2.0f) + (size * 0.35f) + 0.5f);

  switch (align) {
  case ALIGN_LEFT:
    font_draw_string(r.x, y_pos, text, color, size);
    break;
  case ALIGN_CENTER:
    font_draw_string_centered(r.x + (r.w / 2.0f), y_pos, text, color, size);
    break;
  case ALIGN_RIGHT:
    font_draw_string(r.x + r.w - font_get_width(text, size), y_pos, text, color,
                     size);
    break;
  }
}

void ui_draw_hint(const char *text, int x, int y, u32 color) {
  font_draw_string(x, y, text, color, UI_FONT_SIZE_NORMAL);
}

void ui_draw_hint_footer(const char *text, int x, u32 color) {
  font_draw_string(x, 267, text, color, UI_FONT_SIZE_NORMAL);
}

void ui_draw_standard_hints(void) {
    int y = 267;
    u32 col = COLOR_SUBTEXT;
    float sz = UI_FONT_SIZE_TINY;
    const char* text = i18n_get(MSG_HINT_HELPER);

    float text_w = font_get_width(text, sz);
    int x = 240 - (int)(text_w / 2.0f);

    font_draw_string(x, y, text, col, sz);
}

void ui_draw_title(const char *text, Rect r, const ImageResource *icon,
                   int custom_icon_size) {
  int text_x = r.x;
  float text_size = UI_FONT_SIZE_TITLE_MAIN;

  float text_w = font_get_width(text, text_size);

  int icon_size =
      (custom_icon_size > 0) ? custom_icon_size : UI_ICON_SIZE_TITLE;
  int spacing = 12;

  int baseline_y = r.y + 8;

  // The key to perfect alignment: use the same center pivot as ui_draw_text
  // In ui_draw_text: baseline = center + (size * 0.35f) So: center =
  // baseline - (size * 0.35f)
  float text_center_y = baseline_y - (text_size * 0.35f);

  int total_w = (int)text_w + (icon ? (icon_size + spacing) : 0);

  if (icon) {
    // Center icon based on text visual center
    int icon_y = (int)(text_center_y - (icon_size / 2.0f));
    sceGuColor(COLOR_ACCENT);
    texture_draw_resource(icon, r.x, icon_y, icon_size, icon_size);
    text_x += icon_size + spacing;
  }

  font_draw_string(text_x, baseline_y, text, COLOR_ACCENT, text_size);

  // Line below title
  renderer_draw_rect(r.x, baseline_y + 10, total_w, 2, COLOR_ACCENT);
}

void ui_draw_title_auto(const char *text, Rect r, const ImageResource *icon) {
  ui_draw_title(text, r, icon, -1);
}

void ui_draw_app_header(Rect r) {
  const char *title = APP_TITLE;
  float text_size = UI_FONT_SIZE_TITLE_MAIN;
  int spacing = 12;
  int icon_size = UI_ICON_SIZE_LOGO;

  char part1[64];
  char part2[64];

  ui_text_utf8_split_smart(title, part1, part2, sizeof(part1), sizeof(part2), text_size);

  float w1 = font_get_width(part1, text_size);
  float w2 = font_get_width(part2, text_size);
  float total_text_w = w1 + (part2[0] != '\0' ? w2 : 0);
  int total_w = icon_size + spacing + (int)total_text_w;

  int baseline_y = r.y + 8;
  float text_center_y = baseline_y - (text_size * 0.35f);
  int icon_y = (int)(text_center_y - (icon_size / 2.0f));

  // 1. Draw Logo
  sceGuColor(COLOR_TEXT);
  texture_draw_resource(&GD_IMG_ICON_LOGO_32_PNG, r.x, icon_y, icon_size, icon_size);

  // 2. Draw Title (Bipartide)
  int cur_x = r.x + icon_size + spacing;
  font_draw_string(cur_x, baseline_y, part1, COLOR_TEXT, text_size);

  if (part2[0] != '\0') {
    cur_x += (int)w1;
    font_draw_string(cur_x, baseline_y, part2, COLOR_ACCENT, text_size);
  }

  // 3. Line below
  renderer_draw_rect(r.x, baseline_y + 10, total_w, 2, COLOR_ACCENT);
}

/* -----------------------------------------------------------------------
 * Graph Drawing Utilities (Layer 1)
 * Reusable, lightweight functions to prevent duplication in graph rendering
 * without adding complex indirection/configurations.
 * ----------------------------------------------------------------------- */

static float ui_graph_scale(u32 value, u32 max, int max_height) {
  if (max == 0) return 0.0f;
  return ((float)value / (float)max) * (float)max_height;
}

static float ui_graph_lerp(float current, float target, float speed) {
  return current + (target - current) * speed;
}

static void ui_graph_draw_bar(int x, int baseline_y, int w, int h, u32 color) {
  if (h > 0) {
    renderer_draw_rect(x, baseline_y - h, w, h, color);
  }
}

static void ui_graph_draw_gloss(int x, int baseline_y, int w, int h, u8 gloss_alpha) {
  if (h > 4 && gloss_alpha > 0) {
    renderer_draw_rect(x, baseline_y - h, w, 2, ((u32)gloss_alpha << 24) | 0x00FFFFFFu);
  }
}

static void draw_bar_column(int x, int baseline_y, int bar_w, int h,
                            u32 bar_color, u8 gloss_alpha,
                            const char* top_text, float top_size, u32 top_color,
                            const char* btm_text, float btm_size, u32 btm_color) {
  if (h > 0) {
    ui_graph_draw_bar(x, baseline_y, bar_w, h, bar_color);
    ui_graph_draw_gloss(x, baseline_y, bar_w, h, gloss_alpha);

    if (top_text) {
      Rect top_rect = {x - 8, baseline_y - h - 16, bar_w + 16, 12};
      ui_draw_text(top_text, top_rect, top_color, top_size, ALIGN_CENTER);
    }
  }

  if (btm_text) {
    Rect btm_rect = {x - 8, baseline_y + 4, bar_w + 16, 12};
    ui_draw_text(btm_text, btm_rect, btm_color, btm_size, ALIGN_CENTER);
  }
}

void ui_draw_stats_graph(const StatsGraphData *data, int center_x, int baseline_y, int total_w, int max_bar_h) {
  if (!data || data->column_count <= 0) return;

  int count = data->column_count;
  if (count > MAX_GRAPH_COLS) count = MAX_GRAPH_COLS;

  int bar_w = 20;
  if (count <= 7) {
      bar_w = 26; // Weekly
  } else if (count <= 12) {
      bar_w = 18; // Last 12 Months - narrower bars to avoid overlap
  } else {
      // Monthly view needs thinner bars
      bar_w = total_w / count - 2;
      if (bar_w < 4) bar_w = 4;
  }

  int spacing = (total_w - (count * bar_w)) / (count > 1 ? count - 1 : 1);
  if (spacing < 1) spacing = 1;

  // Real total width based on calculation
  int graph_w = (count * bar_w) + ((count - 1) * spacing);
  int gx = center_x - (graph_w / 2);
  int gy = baseline_y;

  // Draw Base Line
  renderer_draw_rect(gx - 10, gy, graph_w + 20, 1, COLOR_BORDER);

  // Check empty state
  int found_any = 0;
  for (int i = 0; i < count; i++) {
      if (data->column_values[i] > 0) {
          found_any = 1;
          break;
      }
  }

  if (!found_any) {
    Rect msg_rect = {gx, gy - 50, graph_w, 30};
    ui_draw_text(i18n_get(MSG_STATS_NO_ACTIVITY), msg_rect, COLOR_SUBTEXT, UI_FONT_SIZE_NORMAL, ALIGN_CENTER);
  }

  // Draw Total Playtime (above graph, near title)
  if (data->context_subtitle[0] != '\0') {
      Rect total_rect = {gx, gy - max_bar_h - 40, graph_w, 20};
      ui_draw_text(data->context_subtitle, total_rect, COLOR_ACCENT, UI_FONT_SIZE_MEDIUM, ALIGN_CENTER);
  }

  // Draw Context String (e.g. "Apr 2026")
  if (data->context_title[0] != '\0') {
      Rect sub_rect = {gx, gy + 25, graph_w, 20};
      ui_draw_text(data->context_title, sub_rect, COLOR_SUBTEXT, UI_FONT_SIZE_SMALL, ALIGN_CENTER);
  }

  // Find Peak Index
  int peak_idx = -1;
  u32 max_v = 0;
  for (int j = 0; j < count; j++) {
    if (data->column_values[j] >= max_v && data->column_values[j] > 0) {
      max_v = data->column_values[j];
      peak_idx = j;
    }
  }

  // Rendering Columns
  for (int i = 0; i < count; i++) {
    float target_h = ui_graph_scale(data->column_values[i], data->max_value, max_bar_h);

    g_graph_anim_h[i] = ui_graph_lerp(g_graph_anim_h[i], target_h, 0.08f);
    g_graph_anim_x[i] = ui_graph_lerp(g_graph_anim_x[i], 0.0f, 0.12f);
    g_graph_anim_a[i] = ui_graph_lerp(g_graph_anim_a[i], 255.0f, 0.08f);

    int target_base_x = gx + i * (bar_w + spacing);
    int x = target_base_x + (int)g_graph_anim_x[i];
    int h = (int)g_graph_anim_h[i];
    if (h < 2 && data->column_values[i] > 0) h = 2;

    int alpha = (int)g_graph_anim_a[i];
    if (alpha) {
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;
    }

    u32 base_bar_color = (i == count - 1) ? COLOR_ACCENT : (COLOR_ACCENT & 0x44FFFFFF);
    u32 label_color = (i == count - 1) ? COLOR_ACCENT : COLOR_SUBTEXT;
    u8 gloss_a = (i == count - 1) ? 0x88 : 0x22;

    u32 bar_color = (base_bar_color & 0x00FFFFFF) | ((((base_bar_color >> 24) * alpha) / 255) << 24);
    u32 fade_label = (label_color & 0x00FFFFFF) | ((u32)alpha << 24);
    u8 fade_gloss = (u8)(((int)gloss_a * alpha) / 255);

    // Value text
    char time_buf[16] = {0};
    if (h > 0) {
        bool show_v = false;
        if (count <= 10) show_v = true;
        // Show peak and last column values always
        else if (i == peak_idx || i == count - 1) show_v = true;
        // Show more values: every 3rd column if not adjacent to peak/last
        else if (i % 3 == 0 && abs(i - peak_idx) > 1 && abs(i - (count - 1)) > 1) show_v = true;

        if (show_v) {
            ui_format_duration(data->column_values[i], time_buf, sizeof(time_buf));
        }
    }

    // Label text
    char label_buf[16] = {0};
    if (data->query.period == STATS_PERIOD_WEEKLY) {
        struct tm bar_tm = *localtime(&data->column_dates[i]);
        snprintf(label_buf, sizeof(label_buf), "%s", i18n_get(MSG_DAY_SUN + bar_tm.tm_wday));
    } else if (data->query.period == STATS_PERIOD_MONTHLY) {
        struct tm bar_tm = *localtime(&data->column_dates[i]);
        if (i == 0 || bar_tm.tm_mday % 5 == 0 || i == count - 1) {
            snprintf(label_buf, sizeof(label_buf), "%d", bar_tm.tm_mday);
        }
    } else if (data->query.period == STATS_PERIOD_LAST_12_MONTHS) {
        struct tm bar_tm = *localtime(&data->column_dates[i]);
        const char* full_name = i18n_get(MSG_MONTH_JAN + bar_tm.tm_mon);
        // Shorten to first 3 characters (approx 3-6 bytes)
        strncpy(label_buf, full_name, 15);
        label_buf[15] = '\0';
        if (strlen(label_buf) > 3) {
            // Find 3rd character boundary (simple check for ASCII/Latin)
            int bytes = 0;
            int chars = 0;
            while (label_buf[bytes] && chars < 3) {
                if ((label_buf[bytes] & 0xC0) != 0x80) chars++;
                bytes++;
            }
            label_buf[bytes] = '\0';
        }
    } else if (data->query.period == STATS_PERIOD_YEARLY) {
        struct tm bar_tm = *localtime(&data->column_dates[i]);
        int y = bar_tm.tm_year + 1900;
        snprintf(label_buf, sizeof(label_buf), "'%02d", y % 100);
    }

    draw_bar_column(x, gy, bar_w, h, bar_color, fade_gloss,
                    time_buf[0] != '\0' ? time_buf : NULL, UI_FONT_SIZE_PICO, fade_label,
                    label_buf[0] != '\0' ? label_buf : NULL, UI_FONT_SIZE_TINY, fade_label);
  }
}

void ui_reset_stats_graph_animation(void) {
  memset(g_graph_anim_h, 0, sizeof(g_graph_anim_h));
  for (int i = 0; i < MAX_GRAPH_COLS; i++) {
     g_graph_anim_x[i] = -15.0f;
     g_graph_anim_a[i] = 0.0f;
  }
}

/* -----------------------------------------------------------------------
 * Session Bar Graph
 *
 * Shows the last MAX_SESSION_BARS sessions for ONE specific game.
 * Inspiration: Steam's "recent playtime" activity chart.
 *   - Bars ordered oldest (left) → newest (right).
 *   - Opacity gradient: oldest bars are dim, newest bar is full accent.
 *   - Heights animated with a lerp grow-in on first draw.
 * ----------------------------------------------------------------------- */
#define MAX_SESSION_BARS 8

static float g_session_anim[MAX_SESSION_BARS] = {0.0f};

void ui_reset_game_daily_graph_animation(void) {
  memset(g_session_anim, 0, sizeof(g_session_anim));
}

typedef struct {
  time_t day_start; /* Normalized to local midnight */
  u32 duration;
} DailyPlayData;

static int compute_game_daily_data(const SessionEntry *sessions, int count,
                                   u32 game_uid, int max_days,
                                   DailyPlayData aggregated[], u32 *max_dur_out) {
  /* Step 1: Collect unique "day starts" that any session overlaps.
   * We use the session END time (timestamp + duration) to detect midnight
   * crossings and register both the start day and any subsequent days. */
  time_t day_candidates[MAX_SESSION_BARS * 2]; /* generous upper-bound */
  int cand_count = 0;

  for (int i = 0; i < count; i++) {
    if (sessions[i].game_uid != game_uid || sessions[i].duration == 0)
      continue;

    time_t s_start = (time_t)sessions[i].timestamp;
    time_t s_end   = s_start + (time_t)sessions[i].duration;

    /* Walk through each calendar day that this session touches */
    struct tm day_tm = *localtime(&s_start);
    day_tm.tm_hour = 0; day_tm.tm_min = 0; day_tm.tm_sec = 0;
    time_t day_cur = mktime(&day_tm);

    while (day_cur < s_end) {
      /* Register this day if not already present */
      int found = 0;
      for (int d = 0; d < cand_count; d++) {
        if (day_candidates[d] == day_cur) { found = 1; break; }
      }
      if (!found && cand_count < MAX_SESSION_BARS * 2) {
        day_candidates[cand_count++] = day_cur;
      }
      day_cur += 86400; /* advance one calendar day */
    }
  }

  if (cand_count == 0) {
    *max_dur_out = 60u;
    return 0;
  }

  /* Step 2: Sort day_candidates descending (most recent first) so we can
   * pick the last max_days unique days. Simple insertion sort — small N. */
  for (int i = 1; i < cand_count; i++) {
    time_t key = day_candidates[i];
    int j = i - 1;
    while (j >= 0 && day_candidates[j] < key) {
      day_candidates[j + 1] = day_candidates[j];
      j--;
    }
    day_candidates[j + 1] = key;
  }

  /* Keep only the most recent max_days days */
  int day_count = (cand_count < max_days) ? cand_count : max_days;

  /* Step 3: For each selected day, accumulate split duration via time_overlap.
   * Iterate in reverse so aggregated[0] = oldest, aggregated[day_count-1] = newest. */
  for (int d = 0; d < day_count; d++) {
    /* day_candidates[0] is newest; day_candidates[day_count-1] is oldest */
    time_t day_start = day_candidates[day_count - 1 - d];
    time_t day_end   = day_start + 86400;

    aggregated[d].day_start = day_start;
    aggregated[d].duration  = 0;

    for (int i = 0; i < count; i++) {
      if (sessions[i].game_uid != game_uid || sessions[i].duration == 0)
        continue;
      time_t s_start = (time_t)sessions[i].timestamp;
      time_t s_end   = s_start + (time_t)sessions[i].duration;
      aggregated[d].duration += utils_time_overlap_secs(s_start, s_end, day_start, day_end);
    }
  }

  /* Step 4: Find max duration for bar scaling */
  u32 max_dur = 60u;
  for (int i = 0; i < day_count; i++) {
    if (aggregated[i].duration > max_dur)
      max_dur = aggregated[i].duration;
  }
  *max_dur_out = max_dur;

  return day_count;
}

void ui_draw_game_daily_graph(const SessionEntry *sessions, int count,
                               u32 game_uid, int max_bars,
                               int center_x, int baseline_y, int max_bar_h) {
  if (max_bars <= 0 || max_bars > MAX_SESSION_BARS)
    max_bars = MAX_SESSION_BARS;

  DailyPlayData daily_data[MAX_SESSION_BARS];
  u32 max_dur;
  int day_count = compute_game_daily_data(sessions, count, game_uid, max_bars,
                                          daily_data, &max_dur);

  if (day_count == 0)
    return; /* No activity — draw nothing. */

  /* Layout */
  int bar_w = 24;
  int gap = 16;
  int total_w = day_count * bar_w + (day_count - 1) * gap;
  int gx = center_x - (total_w / 2);

  /* Baseline separator */
  renderer_draw_rect(gx - 10, baseline_y, total_w + 20, 1, COLOR_BORDER);

  /* Rendering */
  for (int b = 0; b < day_count; b++) {
    u32 dur = daily_data[b].duration;
    float target_h = ui_graph_scale(dur, max_dur, max_bar_h);

    g_session_anim[b] = ui_graph_lerp(g_session_anim[b], target_h, 0.09f);
    int h = (int)g_session_anim[b];
    if (h < 2 && dur > 0)
      h = 2;

    int bx = gx + b * (bar_w + gap);

    float recency = (float)(b + 1) / (float)day_count;
    u8 alpha = (u8)(64.0f + 191.0f * recency);
    u8 gloss_a = alpha >> 1;
    u32 bar_color = ((u32)alpha << 24) | (COLOR_ACCENT & 0x00FFFFFFu);

    // Value text
    char dur_buf[16] = {0};
    if (h > 0) {
      ui_format_duration(dur, dur_buf, sizeof(dur_buf));
    }
    u32 lbl_color = (b == day_count - 1) ? COLOR_ACCENT : COLOR_SUBTEXT;

    // Date text
    struct tm ts_tm = *localtime(&daily_data[b].day_start);
    char date_buf[16];
    strftime(date_buf, sizeof(date_buf), i18n_get(MSG_DATE_FORMAT_SHORT),
             &ts_tm);

    draw_bar_column(bx, baseline_y, bar_w, h, bar_color, gloss_a,
                    h > 0 ? dur_buf : NULL, UI_FONT_SIZE_PICO, lbl_color, date_buf, UI_FONT_SIZE_PICO,
                    COLOR_SUBTEXT);
  }
}


void ui_draw_menu_item(int x, int y, int w, int h, const char *label,
                       bool selected, const ImageResource *left_icon,
                       const ImageResource *right_icon, int custom_icon_size) {
  (void)x;
  (void)y;
  (void)w;
  (void)h; // Keep parameters for potential future layout use

  // 1. Background Highlight
  if (selected) {
    // Draw selected background (slightly lighter than card, or with accent
    // border)
    renderer_draw_rect(x, y, w, h, COLOR_HIGHLIGHT);
    renderer_draw_rect(x, y, 3, h,
                       COLOR_ACCENT); // Accent indicator on the left
  }

  u32 text_color = selected ? COLOR_ACCENT : COLOR_TEXT;
  u32 icon_color = selected ? COLOR_ACCENT : COLOR_SUBTEXT;

  int current_x = x + 12;
  int center_y = y + (h / 2);

  int icon_size = (custom_icon_size > 0) ? custom_icon_size : UI_ICON_SIZE_MENU;

  // 2. Left Icon
  if (left_icon) {
    int icon_y = center_y - (icon_size / 2);

    sceGuColor(icon_color);
    texture_draw_resource(left_icon, current_x, icon_y, icon_size, icon_size);
    current_x += icon_size + 10;
  }

  // 3. Label
  Rect text_rect = {current_x, y, w - (current_x - x) - 40, h};
  ui_draw_text_auto_fit(label, text_rect, text_color, UI_FONT_SIZE_PRIMARY, ALIGN_LEFT);

  // 4. Right Icon (often used for flags or arrows)
  if (right_icon) {
    int icon_x = x + w - icon_size - 12;
    int icon_y = center_y - (icon_size / 2);

    // Flags don't usually get tinted (keep original colors),
    // but system icons like arrows do.
    // We use icon_color for system icons. For flags, we force white (no tint).
    // A simple check: if icon color is selected accent, tint it.
    // If it's a flag (full color), we might want to skip tinting.
    // For now, let's assume we tint if it's a "system" icon.
    // Actually, to make it flexible, let's tint if it's white in source.

    // Design choice: only tint if the caller handles it or if it's not a flag.
    // We'll trust sceGuColor state.
    sceGuColor(0xFFFFFFFF); // Default to white for flags/colored assets
    if (right_icon == &GD_IMG_ICON_SETTINGS_128_PNG ||
        right_icon == &GD_IMG_ICON_SETTINGS_32_PNG ||
        right_icon == &GD_IMG_ICON_LANGUAGE_128_PNG ||
        right_icon == &GD_IMG_ICON_LANGUAGE_32_PNG) {
      sceGuColor(icon_color);
    }

    texture_draw_resource(right_icon, icon_x, icon_y, icon_size, icon_size);
  }
}

void ui_draw_menu_item_auto(int x, int y, int w, int h, const char *label,
                            bool selected, const ImageResource *left_icon,
                            const ImageResource *right_icon) {
  ui_draw_menu_item(x, y, w, h, label, selected, left_icon, right_icon, -1);
}

void ui_format_duration(u32 seconds, char *out, size_t size) {
  u32 d = seconds / 86400;
  u32 h = (seconds % 86400) / 3600;
  u32 m = (seconds % 3600) / 60;

  if (d > 0) {
    snprintf(out, size, i18n_get(MSG_DURATION_D_H_M), d, h, m);
  } else if (h > 0) {
    if (m > 0) {
      snprintf(out, size, i18n_get(MSG_DURATION_H_M), h, m);
    } else {
      snprintf(out, size, i18n_get(MSG_DURATION_HOURS), h);
    }
  } else {
    snprintf(out, size, i18n_get(MSG_DURATION_MINS), m);
  }
}
