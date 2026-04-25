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
 * @file ui_text.c
 * @brief UI text utilities implementation.
 */

#include "app/ui/ui_text.h"
#include "app/render/font.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/**
 * @brief Returns the size of a UTF-8 character based on its lead byte.
 */
size_t utf8_char_size(unsigned char c) {
  if ((c & 0x80) == 0x00) return 1;
  if ((c & 0xE0) == 0xC0) return 2;
  if ((c & 0xF0) == 0xE0) return 3;
  if ((c & 0xF8) == 0xF0) return 4;
  return 1;
}

/**
 * @brief Internal ASCII check for semantic breakpoint detection.
 */
static int is_ascii_internal(unsigned char c) {
  return c < 128;
}

void ui_text_utf8_split_smart(const char *src,
                              char *out1, char *out2,
                              size_t out1_size, size_t out2_size,
                              float text_size) {
  if (!src || !out1 || !out2 || out1_size == 0 || out2_size == 0) return;

  float total_width = font_get_width(src, text_size);
  float target = total_width / 2.0f;

  float acc_width = 0.0f;
  size_t i = 0;

  size_t best_split = 0;
  float best_diff = 1e9f;
  int found_safe = 0;

  char temp[8];

  while (src[i]) {
    size_t sz = utf8_char_size((unsigned char)src[i]);

    memcpy(temp, &src[i], sz);
    temp[sz] = '\0';

    float w = font_get_width(temp, text_size);

    size_t next_i = i + sz;
    unsigned char next_c = (unsigned char)src[next_i];

    int safe_split = 0;

    // 1. Space
    if (next_c != '\0' && next_c == ' ') {
      safe_split = 1;
    }

    // 2. CamelCase (a -> A)
    if (next_c != '\0' &&
        is_ascii_internal((unsigned char)src[i]) &&
        is_ascii_internal(next_c)) {
      if (islower((unsigned char)src[i]) && isupper(next_c)) {
        safe_split = 1;
      }
    }

    acc_width += w;

    if (safe_split) {
      float diff = acc_width - target;
      if (diff < 0) diff = -diff;

      if (!found_safe || diff < best_diff) {
        best_diff = diff;
        best_split = next_i;
        found_safe = 1;
      }
    }

    i = next_i;
  }

  // --- fallback ---
  if (!found_safe) {
    acc_width = 0.0f;
    i = 0;

    while (src[i]) {
      size_t sz = utf8_char_size((unsigned char)src[i]);

      memcpy(temp, &src[i], sz);
      temp[sz] = '\0';

      float w = font_get_width(temp, text_size);

      if ((acc_width + (w / 2.0f)) > target) {
        best_split = i;
        break;
      }

      acc_width += w;
      i += sz;
    }

    if (src[i] == '\0') {
      best_split = i;
    }
  }

  // --- copy part 1 ---
  size_t len1 = best_split;
  if (len1 >= out1_size) len1 = out1_size - 1;

  memcpy(out1, src, len1);
  out1[len1] = '\0';

  // --- copy part 2 ---
  const char *p2 = src + best_split;

  // skip initial space
  if (*p2 == ' ') p2++;

  size_t len2 = strlen(p2);
  if (len2 >= out2_size) len2 = out2_size - 1;

  memcpy(out2, p2, len2);
  out2[len2] = '\0';
}

void ui_text_ellipsis(const char *src, char *dst, size_t dst_size, float scale,
                      float max_width) {
  if (!src || !dst || dst_size == 0)
    return;

  // 1. Initial check: Does the full string fit?
  if (font_get_width(src, scale) <= max_width) {
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
    return;
  }

  // 2. Truncation: Binary search for the cutoff point
  const char *ellipsis = "...";
  float ellipsis_w = font_get_width(ellipsis, scale);
  float target_w = max_width - ellipsis_w;

  if (target_w < 0) {
    // If even the ellipsis doesn't fit, just return empty or ellipsis if it fits
    if (ellipsis_w <= max_width) {
       strncpy(dst, ellipsis, dst_size - 1);
       dst[dst_size - 1] = '\0';
    } else {
       dst[0] = '\0';
    }
    return;
  }

  size_t len = strlen(src);
  size_t low = 0;
  size_t high = len;
  size_t best_fit = 0;

  char temp[128]; // Stack buffer for measurement
  if (dst_size < 128) {
      // Use smaller of dst_size and our measurement buffer
  }

  while (low <= high) {
    size_t mid = low + (high - low) / 2;

    // Safety check for buffer
    size_t test_len = mid;
    if (test_len >= sizeof(temp)) test_len = sizeof(temp) - 1;

    snprintf(temp, sizeof(temp), "%.*s", (int)test_len, src);
    float w = font_get_width(temp, scale);

    if (w <= target_w) {
      best_fit = test_len;
      low = mid + 1;
    } else {
      if (mid == 0) break;
      high = mid - 1;
    }
  }

  // 3. Construct result
  snprintf(dst, dst_size, "%.*s%s", (int)best_fit, src, ellipsis);
}

void ui_draw_text_auto_fit(const char *text, Rect r, u32 color, float size,
                           UIAlign align) {
  if (!text)
    return;

  float current_w = font_get_width(text, size);
  float final_scale = size;
  char buffer[256];
  const char *draw_ptr = text;

  // 1. Try at initial scale
  if (current_w > (float)r.w) {
    // 2. Calculate proportional scale
    float needed_scale = ((float)r.w / current_w) * size;

    // 3. Limit scale >= 0.8 * initial size
    float min_scale = size * 0.8f;
    if (needed_scale < min_scale) {
      final_scale = min_scale;
      // 4. If still overflows, apply ellipsis
      ui_text_ellipsis(text, buffer, sizeof(buffer), final_scale, (float)r.w);
      draw_ptr = buffer;
    } else {
      final_scale = needed_scale;
    }
  }

  ui_draw_text(draw_ptr, r, color, final_scale, align);
}

void ui_draw_game_name(const char *text, Rect r, u32 color, float size, UIAlign align) {
  if (!text || !*text) return;
  float y_pos = floorf(r.y + (r.h / 2.0f) + (size * 0.35f) + 0.5f);
  int ialign = 0;
  if (align == ALIGN_CENTER) ialign = 1;
  else if (align == ALIGN_RIGHT) ialign = 2;
  
  if (ialign == 0) {
    font_draw_game_name(r.x, y_pos, text, color, size, ialign);
  } else if (ialign == 1) {
    font_draw_game_name(r.x + (r.w / 2.0f), y_pos, text, color, size, ialign);
  } else {
    font_draw_game_name(r.x + r.w, y_pos, text, color, size, ialign);
  }
}

void ui_draw_game_name_auto_fit(const char *text, Rect r, u32 color, float size,
                                UIAlign align) {
  if (!text || !*text)
    return;

  float current_w = font_get_game_name_width(text, size);
  float final_scale = size;
  char buffer[256];
  const char *draw_ptr = text;

  if (current_w > (float)r.w) {
    float needed_scale = ((float)r.w / current_w) * size;
    float min_scale = size * 0.8f;
    if (needed_scale < min_scale) {
      final_scale = min_scale;
      
      // Ellipsis logic adapted for font_get_game_name_width
      const char *ellipsis = "...";
      float ellipsis_w = font_get_game_name_width(ellipsis, final_scale);
      float target_w = (float)r.w - ellipsis_w;
      
      if (target_w < 0) {
        if (ellipsis_w <= (float)r.w) {
          strncpy(buffer, ellipsis, sizeof(buffer) - 1);
          buffer[sizeof(buffer) - 1] = '\0';
        } else {
          buffer[0] = '\0';
        }
      } else {
        size_t best_fit = 0;
        char temp[128];
        size_t curr_len = 0;
        size_t max_len = strlen(text);
        
        while (curr_len < max_len) {
          size_t c_size = utf8_char_size((unsigned char)text[curr_len]);
          if (c_size == 0) c_size = 1; // Fallback

          size_t next_len = curr_len + c_size;
          if (next_len >= sizeof(temp)) break;

          snprintf(temp, sizeof(temp), "%.*s", (int)next_len, text);
          float w = font_get_game_name_width(temp, final_scale);

          if (w <= target_w) {
            best_fit = next_len;
            curr_len = next_len;
          } else {
            break;
          }
        }
        snprintf(buffer, sizeof(buffer), "%.*s%s", (int)best_fit, text, ellipsis);
      }
      draw_ptr = buffer;
    } else {
      final_scale = needed_scale;
    }
  }

  float y_pos = floorf(r.y + (r.h / 2.0f) + (final_scale * 0.35f) + 0.5f);
  int ialign = 0;
  if (align == ALIGN_CENTER) ialign = 1;
  else if (align == ALIGN_RIGHT) ialign = 2;
  
  if (ialign == 0) {
    font_draw_game_name(r.x, y_pos, draw_ptr, color, final_scale, ialign);
  } else if (ialign == 1) {
    font_draw_game_name(r.x + (r.w / 2.0f), y_pos, draw_ptr, color, final_scale, ialign);
  } else {
    font_draw_game_name(r.x + r.w, y_pos, draw_ptr, color, final_scale, ialign);
  }
}

void ui_text_wrap(const char* src, float scale, int max_px_width,
                  char out_lines[][MAX_LINE_WIDTH], int max_lines, int* out_count) {
    if (!src || !out_count) return;

    *out_count = 0;
    if (src[0] == '\0') {
        if (*out_count < max_lines) {
            out_lines[*out_count][0] = '\0';
            (*out_count)++;
        }
        return;
    }

    size_t i = 0;
    while (src[i] != '\0' && *out_count < max_lines) {
        float current_w = 0.0f;
        size_t last_space_i = 0;
        size_t current_segment_start = i;
        int found_space = 0;
        int found_newline = 0;
        char temp_char[5];

        size_t cursor = i;
        size_t last_fitting_cursor = cursor;

        while (src[cursor] != '\0') {
            // Hard break on newline
            if (src[cursor] == '\n') {
                found_newline = 1;
                break;
            }

            size_t sz = utf8_char_size((unsigned char)src[cursor]);
            memcpy(temp_char, &src[cursor], sz);
            temp_char[sz] = '\0';

            float char_w = font_get_width(temp_char, scale);
            if (current_w + char_w > (float)max_px_width && cursor > current_segment_start) {
                break;
            }

            current_w += char_w;
            if (src[cursor] == ' ') {
                found_space = 1;
                last_space_i = cursor;
            }

            last_fitting_cursor = cursor + sz;
            cursor += sz;
        }

        if (found_newline) {
            size_t len = cursor - current_segment_start;
            if (len >= MAX_LINE_WIDTH) len = MAX_LINE_WIDTH - 1;
            memcpy(out_lines[*out_count], &src[current_segment_start], len);
            out_lines[*out_count][len] = '\0';
            (*out_count)++;
            i = cursor + 1; // Skip the newline char
            continue;
        }

        if (src[cursor] == '\0') {
            // Fits entirely in remaining space
            size_t len = cursor - current_segment_start;
            if (len >= MAX_LINE_WIDTH) len = MAX_LINE_WIDTH - 1;
            memcpy(out_lines[*out_count], &src[current_segment_start], len);
            out_lines[*out_count][len] = '\0';
            (*out_count)++;
            break;
        } else {
            // Needs breaking
            size_t break_pt = 0;
            if (found_space) {
                break_pt = last_space_i;
            } else {
                break_pt = last_fitting_cursor;
            }

            size_t len = break_pt - current_segment_start;
            if (len >= MAX_LINE_WIDTH) len = MAX_LINE_WIDTH - 1;
            memcpy(out_lines[*out_count], &src[current_segment_start], len);
            out_lines[*out_count][len] = '\0';
            (*out_count)++;

            i = break_pt;
            if (found_space && src[i] == ' ') i++;
        }
    }
}
