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

static int s_blit_x = 0;
static int s_blit_y = 0;
static u32 s_bg_col = 0xFF000000;
static u32 s_fg_col = 0xFFFFFFFF;
static void *s_vram_base = NULL;
static int s_vram_stride = PSP_LINE_SIZE;
static int s_vram_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
static int s_blit_ready = 0;

extern u8 msx[];

static u16 convert_8888_to_565(u32 color) {
  int b = (int)((color >> 19) & 0x1F);
  int g = (int)((color >> 10) & 0x3F);
  int r = (int)((color >> 3) & 0x1F);
  return (u16)(r | (g << 5) | (b << 11));
}

static u16 convert_8888_to_5551(u32 color) {
  int a = (color >> 24) ? 0x8000 : 0;
  int b = (int)((color >> 19) & 0x1F);
  int g = (int)((color >> 11) & 0x1F);
  int r = (int)((color >> 3) & 0x1F);
  return (u16)(a | r | (g << 5) | (b << 10));
}

static u16 convert_8888_to_4444(u32 color) {
  int a = (int)((color >> 28) & 0xF);
  int b = (int)((color >> 20) & 0xF);
  int g = (int)((color >> 12) & 0xF);
  int r = (int)((color >> 4) & 0xF);
  return (u16)((a << 12) | r | (g << 4) | (b << 8));
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
    fg = convert_8888_to_565(s_fg_col);
    bg = convert_8888_to_565(s_bg_col);
    break;
  case PSP_DISPLAY_PIXEL_FORMAT_5551:
    fg = convert_8888_to_5551(s_fg_col);
    bg = convert_8888_to_5551(s_bg_col);
    break;
  case PSP_DISPLAY_PIXEL_FORMAT_4444:
    fg = convert_8888_to_4444(s_fg_col);
    bg = convert_8888_to_4444(s_bg_col);
    break;
  default:
    fg = convert_8888_to_565(s_fg_col);
    bg = convert_8888_to_565(s_bg_col);
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
