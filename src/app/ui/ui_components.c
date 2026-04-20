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

static float g_graph_anim[7] = {0.0f};

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

  float y_pos = r.y + (r.h / 2.0f) +
                (size * 6.0f); // Simple vertical centering approximation

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
  font_draw_string(x, y, text, color, 0.8f);
}

void ui_draw_hint_footer(const char *text, int x, u32 color) {
  font_draw_string(x, 267, text, color, 0.8f);
}

void ui_draw_standard_hints(void) {
    int y = 267;
    u32 col = COLOR_SUBTEXT;
    float sz = 0.7f;

    const char* labels[] = {
        i18n_get(MSG_CTRL_NAVIGATE),
        i18n_get(MSG_CTRL_SELECT),
        i18n_get(MSG_CTRL_BACK),
        i18n_get(MSG_CTRL_MENU),
        i18n_get(MSG_CTRL_CONFIG)
    };

    const char* icons[] = {
        "[← →]",
        "[X]",
        "[O]",
        "[START]",
        "[SELECT]"
    };

    int count = 5;
    int padding = 5;
    float widths[5];
    float total_width = 0.0f;

    // Calculate total width
    for (int i = 0; i < count; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s %s", icons[i], labels[i]);

        widths[i] = font_get_width(buffer, sz);
        total_width += widths[i];
    }

    float spacing = (480.0f - total_width - (padding * 2)) / (count - 1);
    float x = (float)padding;

    for (int i = 0; i < count; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s %s", icons[i], labels[i]);

        font_draw_string((int)x, y, buffer, col, sz);
        x += widths[i] + spacing;
    }
}

void ui_draw_title(const char *text, Rect r, const ImageResource *icon,
                   int custom_icon_size) {
  int text_x = r.x;
  float text_size = 1.3f;

  float text_w = font_get_width(text, text_size);
  float text_h = font_get_height(text_size);

  int icon_size =
      (custom_icon_size > 0) ? custom_icon_size : (int)(text_h * 0.9f);
  int spacing = 12;

  int baseline_y = r.y + 8;

  // The key to perfect alignment: use the same center pivot as ui_draw_text
  // In ui_draw_text: baseline = center + (size * 6.0f) So: center =
  // baseline - (size * 6.0f)
  float text_center_y = baseline_y - (text_size * 6.0f);

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

static int compute_weekly_data(const SessionEntry *sessions, int count, u32 day_playtime[7], u32 *max_time_out, time_t *today_start_out) {
  u32 now_ts = utils_get_timestamp();
  time_t now_val = (time_t)now_ts;

  struct tm today_tm = *localtime(&now_val);
  today_tm.tm_hour = 0;
  today_tm.tm_min = 0;
  today_tm.tm_sec = 0;
  time_t today_start = mktime(&today_tm);
  *today_start_out = today_start;

  memset(day_playtime, 0, 7 * sizeof(u32));
  int found_any = 0;

  for (int i = 0; i < count; i++) {
    time_t s_time = (time_t)sessions[i].timestamp;
    if (s_time >= (today_start - 6 * 86400) && s_time < (today_start + 86400)) {
      int days_ago;
      if (s_time >= today_start) {
        days_ago = 0;
      } else {
        days_ago = (today_start - s_time) / 86400 + 1;
      }

      if (days_ago >= 0 && days_ago < 7) {
        day_playtime[days_ago] += sessions[i].duration;
        if (sessions[i].duration > 0)
          found_any = 1;
      }
    }
  }

  u32 max_time = 3600; // Min scale 1h
  for (int i = 0; i < 7; i++) {
    if (day_playtime[i] > max_time)
      max_time = day_playtime[i];
  }
  *max_time_out = max_time;

  return found_any;
}

void ui_draw_weekly_graph(SessionEntry *sessions, int count) {
  u32 day_playtime[7];
  u32 max_time;
  time_t today_start;

  int found_any = compute_weekly_data(sessions, count, day_playtime, &max_time, &today_start);

  // Graph Layout
  int gx = 60, gy = 170, gw = 360, gh = 80;
  int bar_w = 30;
  int spacing = (gw - (7 * bar_w)) / 6;

  // Draw Base Line
  renderer_draw_rect(gx - 10, gy, gw + 20, 1, COLOR_BORDER);

  if (!found_any) {
    Rect msg_rect = {gx, gy - 50, gw, 30};
    ui_draw_text(i18n_get(MSG_STATS_NO_ACTIVITY), msg_rect, COLOR_SUBTEXT, 0.8f,
                 ALIGN_CENTER);
  }

  // Rendering
  for (int i = 0; i < 7; i++) {
    int idx = 6 - i; // idx 0 is today (rightmost), idx 6 is 6 days ago (leftmost)
    float target_h = ui_graph_scale(day_playtime[idx], max_time, gh);

    // Simple animation state
    g_graph_anim[idx] = ui_graph_lerp(g_graph_anim[idx], target_h, 0.07f);

    int x = gx + i * (bar_w + spacing);
    int h = (int)g_graph_anim[idx];
    if (h < 2 && day_playtime[idx] > 0)
      h = 2;

    // Visual highlights: Today (idx=0) is bright, others are dimmed
    u32 bar_color = (idx == 0) ? COLOR_ACCENT : (COLOR_ACCENT & 0x44FFFFFF);
    u32 label_color = (idx == 0) ? COLOR_ACCENT : COLOR_SUBTEXT;
    u8 gloss_a = (idx == 0) ? 0x88 : 0x22;

    // Value text
    char time_buf[16] = {0};
    if (h > 0) {
      ui_format_duration(day_playtime[idx], time_buf, sizeof(time_buf));
    }

    // Label text
    time_t bar_day_time = today_start - (idx * 86400);
    struct tm bar_tm = *localtime(&bar_day_time);
    const char *day_name = i18n_get(MSG_DAY_SUN + bar_tm.tm_wday);

    draw_bar_column(x, gy, bar_w, h, bar_color, gloss_a,
                    h > 0 ? time_buf : NULL, 0.6f, label_color,
                    day_name, 0.7f, label_color);
  }
}

void ui_reset_graph_animation(void) {
  memset(g_graph_anim, 0, sizeof(g_graph_anim));
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
  int day_count = 0;
  u32 max_dur = 60u;

  /* Traverse backward through sessions to find the most recent days */
  for (int i = count - 1; i >= 0 && day_count < max_days; i--) {
    if (sessions[i].game_uid != game_uid || sessions[i].duration == 0)
      continue;

    time_t s_time = (time_t)sessions[i].timestamp;
    struct tm ts_tm = *localtime(&s_time);
    ts_tm.tm_hour = 0;
    ts_tm.tm_min = 0;
    ts_tm.tm_sec = 0;
    time_t s_day_start = mktime(&ts_tm);

    /* Check if this session belongs to the current "latest" day we're collecting */
    if (day_count > 0 && aggregated[day_count - 1].day_start == s_day_start) {
      aggregated[day_count - 1].duration += sessions[i].duration;
    } else {
      /* New day found */
      if (day_count < max_days) {
        aggregated[day_count].day_start = s_day_start;
        aggregated[day_count].duration = sessions[i].duration;
        day_count++;
      }
    }
  }

  /* Reverse so oldest is index 0 (left) and newest is index day_count-1 (right) */
  for (int i = 0; i < day_count / 2; i++) {
    DailyPlayData tmp = aggregated[i];
    aggregated[i] = aggregated[day_count - 1 - i];
    aggregated[day_count - 1 - i] = tmp;
  }

  /* Find max duration for scaling */
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
                    h > 0 ? dur_buf : NULL, 0.75f, lbl_color, date_buf, 0.75f,
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

  int icon_size = (custom_icon_size > 0) ? custom_icon_size : (int)(h * 0.5f);

  // 2. Left Icon
  if (left_icon) {
    int icon_y = center_y - (icon_size / 2);

    sceGuColor(icon_color);
    texture_draw_resource(left_icon, current_x, icon_y, icon_size, icon_size);
    current_x += icon_size + 10;
  }

  // 3. Label
  Rect text_rect = {current_x, y, w - (current_x - x) - 40, h};
  ui_draw_text(label, text_rect, text_color, 0.9f, ALIGN_LEFT);

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
    if (right_icon == &GD_IMG_MENU_ICON_PNG ||
        right_icon == &GD_IMG_LANGUAGE_ICON_PNG) {
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
