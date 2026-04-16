#include "app/ui/ui_components.h"
#include "app/i18n.h"
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

void ui_draw_title(const char *text, Rect r, const ImageResource *icon,
                   int custom_icon_size) {
  int text_x = r.x;
  float text_size = 1.3f;

  float text_w = font_get_width(text, text_size);
  float text_h = font_get_height(text_size);

  int icon_size =
      (custom_icon_size > 0) ? custom_icon_size : (int)(text_h * 0.9f);
  int spacing = 12;

  int baseline_y = r.y + 20;

  // The key to perfect alignment: use the same center pivot as ui_draw_text
  // In ui_draw_text: baseline = center + (size * 6.0f) So: center =
  // baseline - (size * 6.0f)
  float text_center_y = baseline_y - (text_size * 6.0f);

  int total_w = (int)text_w + (icon ? (icon_size + spacing) : 0);

  if (icon) {
    // Centraliza o ícone com base no centro visual do texto
    int icon_y = (int)(text_center_y - (icon_size / 2.0f));
    sceGuColor(COLOR_ACCENT);
    texture_draw_resource(icon, r.x, icon_y, icon_size, icon_size);
    text_x += icon_size + spacing;
  }

  font_draw_string(text_x, baseline_y, text, COLOR_ACCENT, text_size);

  // Linha logo abaixo
  renderer_draw_rect(r.x, baseline_y + 10, total_w, 2, COLOR_ACCENT);
}

void ui_draw_title_auto(const char *text, Rect r, const ImageResource *icon) {
  ui_draw_title(text, r, icon, -1);
}

void ui_draw_weekly_graph(SessionEntry *sessions, int count) {
  u32 day_playtime[7] = {0};
  u32 now_ts = utils_get_timestamp();
  time_t now_val = (time_t)now_ts;

  // Normalize to start of today (local midnight)
  // We copy the struct because localtime returns a static pointer which might
  // be overwritten
  struct tm today_tm = *localtime(&now_val);
  today_tm.tm_hour = 0;
  today_tm.tm_min = 0;
  today_tm.tm_sec = 0;
  time_t today_start = mktime(&today_tm);

  int found_any = 0;

  // Aggregate sessions for the last 7 days (including today)
  for (int i = 0; i < count; i++) {
    time_t s_time = (time_t)sessions[i].timestamp;

    // Check if session falls in the [Today-6, Today] window
    // s_time >= today_start - 6 days
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

  // Scaling and rendering
  u32 max_time = 3600; // Min scale 1h
  for (int i = 0; i < 7; i++) {
    if (day_playtime[i] > max_time)
      max_time = day_playtime[i];
  }

  for (int i = 0; i < 7; i++) {
    int idx =
        6 - i; // idx 0 is today (rightmost), idx 6 is 6 days ago (leftmost)
    float target_h = ((float)day_playtime[idx] / max_time) * gh;

    // Simple animation state (could use a timer, but this frame-based approach
    // is common in homebrew)
    g_graph_anim[idx] += (target_h - g_graph_anim[idx]) * 0.07f;

    int x = gx + i * (bar_w + spacing);
    int h = (int)g_graph_anim[idx];
    if (h < 2 && day_playtime[idx] > 0)
      h = 2;

    // Visual highlights: Today (idx=0) is bright, others are dimmed
    u32 bar_color = (idx == 0) ? COLOR_ACCENT : (COLOR_ACCENT & 0x44FFFFFF);
    u32 label_color = (idx == 0) ? COLOR_ACCENT : COLOR_SUBTEXT;

    if (h > 0) {
      renderer_draw_rect(x, gy - h, bar_w, h, bar_color);
      // "Glossy" top edge for today
      if (idx == 0 && h > 4) {
        renderer_draw_rect(x, gy - h, bar_w, 2, 0x88FFFFFF);
      } else if (h > 4) {
        renderer_draw_rect(x, gy - h, bar_w, 2, 0x22FFFFFF);
      }

      // Draw playtime text on top
      char time_buf[16];
      utils_format_duration_compact(day_playtime[idx], time_buf,
                                    sizeof(time_buf));
      Rect time_rect = {x - 5, gy - h - 14, bar_w + 10, 10};
      ui_draw_text(time_buf, time_rect, label_color, 0.6f, ALIGN_CENTER);
    }

    // Labels rotated based on today
    time_t bar_day_time = today_start - (idx * 86400);
    struct tm bar_tm = *localtime(&bar_day_time);
    const char *day_name = i18n_get(MSG_DAY_SUN + bar_tm.tm_wday);

    Rect label_rect = {x, gy + 8, bar_w, 20};
    ui_draw_text(day_name, label_rect, label_color, 0.7f, ALIGN_CENTER);
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

void ui_reset_session_graph_animation(void) {
  memset(g_session_anim, 0, sizeof(g_session_anim));
}

void ui_draw_session_bar_graph(const SessionEntry *sessions, int count,
                               u32 game_uid, int max_bars,
                               int center_x, int baseline_y, int max_bar_h) {
  /* Clamp so we never exceed the static animation buffer. */
  if (max_bars <= 0 || max_bars > MAX_SESSION_BARS)
    max_bars = MAX_SESSION_BARS;

  /* ----------------------------------------------------------------
   * Collect indices of sessions that belong to this game, in order.
   * We only need the LAST max_bars, so we keep a rolling window.
   * ---------------------------------------------------------------- */
  int  matching[MAX_SESSION_BARS];
  int  match_count = 0;

  for (int i = 0; i < count; i++) {
    if (sessions[i].game_uid != game_uid || sessions[i].duration == 0)
      continue;
    /* Slide window: discard oldest when full. */
    if (match_count == max_bars) {
      for (int j = 0; j < max_bars - 1; j++) matching[j] = matching[j + 1];
      matching[max_bars - 1] = i;
    } else {
      matching[match_count++] = i;
    }
  }

  if (match_count == 0) return; /* No sessions — draw nothing. */

  /* ----------------------------------------------------------------
   * Layout: center the graph horizontally around center_x
   * ---------------------------------------------------------------- */
  int bar_w = 40;
  int gap   = 16;
  int total_w = match_count * bar_w + (match_count - 1) * gap;
  int gx    = center_x - (total_w / 2);

  /* Baseline separator */
  renderer_draw_rect(gx - 10, baseline_y, total_w + 20, 1, COLOR_BORDER);

  /* ----------------------------------------------------------------
   * Find maximum duration to scale bar heights.
   * Minimum scale = 60 s so very short sessions still show a sliver.
   * ---------------------------------------------------------------- */
  u32 max_dur = 60u;
  for (int b = 0; b < match_count; b++) {
    u32 d = sessions[matching[b]].duration;
    if (d > max_dur) max_dur = d;
  }

  /* ----------------------------------------------------------------
   * Draw bars
   * ---------------------------------------------------------------- */
  for (int b = 0; b < match_count; b++) {
    u32 dur        = sessions[matching[b]].duration;
    float target_h = ((float)dur / (float)max_dur) * (float)max_bar_h;

    /* Lerp animation: bar height grows toward target each frame. */
    g_session_anim[b] += (target_h - g_session_anim[b]) * 0.09f;
    int h = (int)g_session_anim[b];
    if (h < 2 && dur > 0) h = 2; /* Always render a visible minimum. */

    int bx = gx + b * (bar_w + gap);

    /* Opacity gradient: oldest (b=0) at 25%, newest (b=max-1) at 100%. */
    float recency = (float)(b + 1) / (float)match_count; /* 0..1 */
    u8 alpha = (u8)(64.0f + 191.0f * recency);            /* 64..255 */

    u32 bar_color = ((u32)alpha << 24) | (COLOR_ACCENT & 0x00FFFFFFu);

    /* Bar body */
    if (h > 0) {
      renderer_draw_rect(bx, baseline_y - h, bar_w, h, bar_color);

      /* Glossy top highlight */
      u8 gloss_a = alpha >> 1;
      renderer_draw_rect(bx, baseline_y - h, bar_w, 2,
                         ((u32)gloss_a << 24) | 0x00FFFFFFu);
    }

    /* Duration label above bar */
    char dur_buf[12];
    utils_format_duration_compact(dur, dur_buf, sizeof(dur_buf));
    u32 lbl_color = (b == match_count - 1) ? COLOR_ACCENT : COLOR_SUBTEXT;
    Rect lbl_rect = {bx - 4, baseline_y - h - 14, bar_w + 8, 10};
    ui_draw_text(dur_buf, lbl_rect, lbl_color, 0.55f, ALIGN_CENTER);

    /* Date label below bar */
    time_t s_time = (time_t)sessions[matching[b]].timestamp;
    struct tm ts_tm = *localtime(&s_time);
    char date_buf[16];
    snprintf(date_buf, sizeof(date_buf), "%02d/%02d", ts_tm.tm_mday,
             ts_tm.tm_mon + 1);

    Rect date_rect = {bx - 4, baseline_y + 4, bar_w + 8, 10};
    ui_draw_text(date_buf, date_rect, COLOR_SUBTEXT, 0.55f, ALIGN_CENTER);
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
