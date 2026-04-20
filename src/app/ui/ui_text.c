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
