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
 * @file overlay_blit.c
 * @brief Overlay blit implementation.
 */



#include "plugin/overlay_blit.h"
#include <pspdisplay.h>
#include <string.h>

#define PSP_SCREEN_WIDTH   480
#define PSP_SCREEN_HEIGHT  272
#define PSP_LINE_SIZE      512
#define BLIT_CELL_W        7
#define BLIT_MARGIN_CELL   1
#define BLIT_PAD_X         6
#define BLIT_PAD_Y         4

static int s_blit_x = 0;
static int s_blit_y = 0;
static u32 s_bg_col = 0xFF1A1A1A;
static u32 s_fg_col = 0xFFFFFFFF;
static void *s_vram_base = NULL;
static int s_vram_stride = PSP_LINE_SIZE;
static int s_vram_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
static int s_blit_ready = 0;

extern u8 msx[];

/* PSP 8888 framebuffer layout: 0xAABBGGRR */
static u16 convert_psp8888_to_565(u32 color) {
  int r = (int)((color >> 16) & 0xFF);
  int g = (int)((color >> 8) & 0xFF);
  int b = (int)(color & 0xFF);
  return (u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static u16 convert_psp8888_to_5551(u32 color) {
  int a = (color & 0xFF000000U) ? 0x8000 : 0;
  int r = (int)((color >> 16) & 0xFF);
  int g = (int)((color >> 8) & 0xFF);
  int b = (int)(color & 0xFF);
  return (u16)(a | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

static u16 convert_psp8888_to_4444(u32 color) {
  int a = (int)((color >> 24) & 0xF);
  int r = (int)((color >> 16) & 0xFF);
  int g = (int)((color >> 8) & 0xFF);
  int b = (int)(color & 0xFF);
  return (u16)((a << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4));
}

void overlay_blit_set_colors(u32 fg_argb, u32 bg_argb) {
  s_fg_col = fg_argb;
  s_bg_col = bg_argb;
}

static int blit_line_char_count(const char *msg) {
  int n = 0;
  if (!msg) {
    return 0;
  }
  for (; *msg; msg++) {
    if (*msg != '\n' && *msg != '\r') {
      n++;
    }
  }
  return n;
}

static void blit_calc_box(int line_count, const char *line1, const char *line2,
                          int *box_x, int *box_y, int *box_w, int *box_h) {
  int max_chars = 0;

  if (line_count >= 1 && line1) {
    int n = blit_line_char_count(line1);
    if (n > max_chars) {
      max_chars = n;
    }
  }
  if (line_count >= 2 && line2) {
    int n = blit_line_char_count(line2);
    if (n > max_chars) {
      max_chars = n;
    }
  }
  if (max_chars < 1) {
    max_chars = 1;
  }

  *box_x = BLIT_MARGIN_CELL * BLIT_CELL_W - BLIT_PAD_X;
  if (*box_x < 0) {
    *box_x = 0;
  }
  *box_y = BLIT_MARGIN_CELL * 8 - BLIT_PAD_Y;
  if (*box_y < 0) {
    *box_y = 0;
  }
  *box_w = max_chars * BLIT_CELL_W + BLIT_PAD_X * 2;
  *box_h = line_count * 8 + BLIT_PAD_Y * 2;
  if (*box_x + *box_w > PSP_SCREEN_WIDTH) {
    *box_w = PSP_SCREEN_WIDTH - *box_x;
  }
  if (*box_y + *box_h > PSP_SCREEN_HEIGHT) {
    *box_h = PSP_SCREEN_HEIGHT - *box_y;
  }
}

static void blit_fill_rect_32(int x, int y, int w, int h, u32 color) {
  u32 *vram = (u32 *)s_vram_base;
  int row;

  if (!s_blit_ready || w <= 0 || h <= 0) {
    return;
  }

  for (row = 0; row < h; row++) {
    int ry = y + row;
    u32 *row_ptr;
    int col;

    if (ry < 0 || ry >= PSP_SCREEN_HEIGHT) {
      continue;
    }
    row_ptr = vram + (ry * s_vram_stride) + x;
    for (col = 0; col < w; col++) {
      if (x + col >= 0 && x + col < PSP_SCREEN_WIDTH) {
        row_ptr[col] = color;
      }
    }
  }
}

static void blit_fill_rect_16(int x, int y, int w, int h, u16 color) {
  u16 *vram = (u16 *)s_vram_base;
  int row;

  if (!s_blit_ready || w <= 0 || h <= 0) {
    return;
  }

  for (row = 0; row < h; row++) {
    int ry = y + row;
    u16 *row_ptr;
    int col;

    if (ry < 0 || ry >= PSP_SCREEN_HEIGHT) {
      continue;
    }
    row_ptr = vram + (ry * s_vram_stride) + x;
    for (col = 0; col < w; col++) {
      if (x + col >= 0 && x + col < PSP_SCREEN_WIDTH) {
        row_ptr[col] = color;
      }
    }
  }
}

static void blit_fill_background_box(int line_count, const char *line1,
                                       const char *line2) {
  int bx, by, bw, bh;

  blit_calc_box(line_count, line1, line2, &bx, &by, &bw, &bh);

  if (s_vram_mode == PSP_DISPLAY_PIXEL_FORMAT_8888) {
    blit_fill_rect_32(bx, by, bw, bh, s_bg_col);
    return;
  }

  {
    u16 bg16 = 0;
    switch (s_vram_mode) {
    case PSP_DISPLAY_PIXEL_FORMAT_565:
      bg16 = convert_psp8888_to_565(s_bg_col);
      break;
    case PSP_DISPLAY_PIXEL_FORMAT_5551:
      bg16 = convert_psp8888_to_5551(s_bg_col);
      break;
    case PSP_DISPLAY_PIXEL_FORMAT_4444:
      bg16 = convert_psp8888_to_4444(s_bg_col);
      break;
    default:
      bg16 = convert_psp8888_to_565(s_bg_col);
      break;
    }
    blit_fill_rect_16(bx, by, bw, bh, bg16);
  }
}

static void blit_setup_target(void *vram, int bufferwidth, int pixelformat) {
  if (!vram) {
    return;
  }
  s_vram_base = vram;
  s_vram_stride = (bufferwidth > 0) ? bufferwidth : PSP_LINE_SIZE;
  s_vram_mode = pixelformat;
  s_blit_ready = 1;
}

static void blit_put_char_32(int x, int y, u32 color, u32 bgc, u8 ch) {
  u8 *font = &msx[(int)ch * 8];
  u32 *vram = (u32 *)s_vram_base;
  vram += x;
  vram += y * s_vram_stride;

  for (int row = 0; row < 8; row++) {
    u8 bits = font[row];
    u32 *row_ptr = vram;
    for (int col = 0; col < 8; col++) {
      row_ptr[col] = (bits & (0x80 >> col)) ? color : bgc;
    }
    vram += s_vram_stride;
  }
}

static void blit_put_char_16(int x, int y, u16 color, u16 bgc, u8 ch) {
  u8 *font = &msx[(int)ch * 8];
  u16 *vram = (u16 *)s_vram_base;
  vram += x;
  vram += y * s_vram_stride;

  for (int row = 0; row < 8; row++) {
    u8 bits = font[row];
    u16 *row_ptr = vram;
    for (int col = 0; col < 8; col++) {
      row_ptr[col] = (bits & (0x80 >> col)) ? color : bgc;
    }
    vram += s_vram_stride;
  }
}

static void blit_put_char(int x, int y, u8 ch) {
  if (!s_blit_ready || ch < 0x20) {
    return;
  }

  if (s_vram_mode == PSP_DISPLAY_PIXEL_FORMAT_8888) {
    blit_put_char_32(x, y, s_fg_col, s_bg_col, ch);
    return;
  }

  u16 fg = 0;
  u16 bg = 0;
  switch (s_vram_mode) {
  case PSP_DISPLAY_PIXEL_FORMAT_565:
    fg = convert_psp8888_to_565(s_fg_col);
    bg = convert_psp8888_to_565(s_bg_col);
    break;
  case PSP_DISPLAY_PIXEL_FORMAT_5551:
    fg = convert_psp8888_to_5551(s_fg_col);
    bg = convert_psp8888_to_5551(s_bg_col);
    break;
  case PSP_DISPLAY_PIXEL_FORMAT_4444:
    fg = convert_psp8888_to_4444(s_fg_col);
    bg = convert_psp8888_to_4444(s_bg_col);
    break;
  default:
    fg = convert_psp8888_to_565(s_fg_col);
    bg = convert_psp8888_to_565(s_bg_col);
    break;
  }
  blit_put_char_16(x, y, fg, bg, ch);
}

static void blit_print_line(const char *msg) {
  if (!msg || !s_blit_ready) {
    return;
  }

  for (; *msg; msg++) {
    char c = *msg;
    if (c == '\n' || c == '\r') {
      s_blit_x = 0;
      s_blit_y++;
      continue;
    }
    if (c < 0x20 || c > 0x7E) {
      c = '?';
    }
    blit_put_char(s_blit_x * BLIT_CELL_W, s_blit_y * 8, (u8)c);
    s_blit_x++;
    if (s_blit_x >= 60) {
      s_blit_x = 0;
      s_blit_y++;
    }
  }
}

static void blit_draw_lines_on_target(void *vram, int bufferwidth, int pixelformat,
                                      const char *line1, const char *line2,
                                      int line_count) {
  int stride = bufferwidth;
  if (stride <= 0) {
    stride = PSP_LINE_SIZE;
  }
  (void)stride;

  blit_setup_target(vram, bufferwidth, pixelformat);
  if (!s_blit_ready) {
    return;
  }

  blit_fill_background_box(line_count, line1, line2);

  s_blit_x = BLIT_MARGIN_CELL;
  s_blit_y = BLIT_MARGIN_CELL;
  if (line_count >= 1 && line1 && line1[0]) {
    blit_print_line(line1);
  }
  if (line_count >= 2 && line2 && line2[0]) {
    s_blit_x = BLIT_MARGIN_CELL;
    s_blit_y = BLIT_MARGIN_CELL + 1;
    blit_print_line(line2);
  }
}

static int blit_try_get_framebuf(int sync, void **out_vram, int *out_stride,
                                 int *out_fmt) {
  void *vram = NULL;
  int stride = 0;
  int fmt = 0;

  if (sceDisplayGetFrameBuf(&vram, &stride, &fmt, sync) < 0 || !vram || stride <= 0) {
    return 0;
  }

  *out_vram = vram;
  *out_stride = stride;
  *out_fmt = fmt;
  return 1;
}

void overlay_blit_draw_to(void *vram, int bufferwidth, int pixelformat,
                          const char *line1, const char *line2, int line_count) {
  if (!vram || line_count <= 0) {
    return;
  }
  blit_draw_lines_on_target(vram, bufferwidth, pixelformat, line1, line2,
                            line_count);
}

static int blit_ptr_seen(void *const *seen, int n, void *p) {
  for (int i = 0; i < n; i++) {
    if (seen[i] == p) {
      return 1;
    }
  }
  return 0;
}

void overlay_blit_draw_all(const char *line1, const char *line2, int line_count) {
  void *vram = NULL;
  int stride = 0;
  int fmt = 0;
  int mode = 0;
  int w = 0;
  int h = 0;
  void *seen[4];
  int nseen = 0;

  if (line_count <= 0) {
    return;
  }

  /* missyhud: sync flag from GetMode */
  if (sceDisplayGetMode(&mode, &w, &h) >= 0) {
    if (blit_try_get_framebuf(mode, &vram, &stride, &fmt) &&
        !blit_ptr_seen(seen, nseen, vram)) {
      seen[nseen++] = vram;
      overlay_blit_draw_to(vram, stride, fmt, line1, line2, line_count);
    }
  }

  if (blit_try_get_framebuf(PSP_DISPLAY_SETBUF_IMMEDIATE, &vram, &stride, &fmt) &&
      !blit_ptr_seen(seen, nseen, vram)) {
    seen[nseen++] = vram;
    overlay_blit_draw_to(vram, stride, fmt, line1, line2, line_count);
  }

  if (blit_try_get_framebuf(PSP_DISPLAY_SETBUF_NEXTFRAME, &vram, &stride, &fmt) &&
      !blit_ptr_seen(seen, nseen, vram)) {
    overlay_blit_draw_to(vram, stride, fmt, line1, line2, line_count);
  }
}

void overlay_blit_draw_fallbacks(const char *line1, const char *line2, int line_count) {
  (void)line1;
  (void)line2;
  (void)line_count;
  /* Intentionally empty: blind EDRAM writes crash many retail games. */
}
