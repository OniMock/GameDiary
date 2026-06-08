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
 * @file overlay_notification.c
 * @brief In-game tracker messages (universal HUD-style blit + display hooks).
 *
 * Follows the same approach as missyhud / FPS plugins:
 * - PSPSDK msx font + GetMode/GetFrameBuf blit
 * - Hooks on sceDisplaySetFrameBuf and internal flip (draw after flip only)
 * - Colored bar: dark green (tracker ON), dark red (tracker OFF)
 */

#include "plugin/overlay_notification.h"
#include "plugin/overlay_blit.h"
#include "common/utils.h"
#include <pspsdk/systemctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

#define OVERLAY_HIDE_MS     2000U
#define OVERLAY_MAX_LINES   2
#define OVERLAY_LINE_LEN    48
#define OVERLAY_TRACKED_FB  4

/* 6.60+ / 6.61 sceDisplay_driver */
#define NID_SET_FRAME_BUF          0xA38B3F89U
#define NID_SET_FRAME_BUF_LEGACY   0x289D82FEU
/* sceDisplay_driver_63E22A26 — internal SetFrameBuf (GTA and others) */
#define NID_SET_FRAME_BUF_INTERNAL 0x3E17FE8DU
/* PSP 8888: 0xAABBGGRR — R in byte 0, G in byte 1, B in byte 2 */
#define OVERLAY_MAKE_COLOR(r, g, b) \
  ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | 0xFF000000U)

#define OVERLAY_COLOR_FG           OVERLAY_MAKE_COLOR(0xFF, 0xFF, 0xFF)
#define OVERLAY_COLOR_BG_ON        OVERLAY_MAKE_COLOR(0x0B, 0x4A, 0x14)
#define OVERLAY_COLOR_BG_OFF       OVERLAY_MAKE_COLOR(0x4A, 0x0B, 0x0B)
#define OVERLAY_COLOR_BG_NEUTRAL   OVERLAY_MAKE_COLOR(0x1A, 0x1A, 0x1A)

static char s_lines[OVERLAY_MAX_LINES][OVERLAY_LINE_LEN];
static u32 s_visible_until_ms = 0;
static int s_line_count = 0;

typedef struct {
  void *vram;
  int stride;
  int format;
} OverlayFbInfo;

static OverlayFbInfo s_tracked[OVERLAY_TRACKED_FB];
static int s_tracked_count = 0;

static int (*s_orig_set_frame_buf)(void *fb, int bufferwidth, int pixelformat,
                                   int sync) = NULL;
static int (*s_orig_set_frame_buf_internal)(int pri, void *fb, int bufferwidth,
                                             int pixelformat, int sync) = NULL;

static FunctionPatchData s_setfb_patch;
static FunctionPatchData s_setfb_internal_patch;
static int (*s_orig_wait_vblank_start)(void) = NULL;
static FunctionPatchData s_vblank_patch;
static int s_hooks_installed = 0;

static void overlay_sanitize_line(char *dst, size_t cap, const char *src) {
  size_t j = 0;
  if (!dst || cap == 0 || !src) {
    return;
  }

  for (size_t i = 0; src[i] && j + 1 < cap; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c >= 0x20 && c <= 0x7E) {
      dst[j++] = (char)c;
    } else if (c >= 0x80) {
      while (src[i + 1] && ((unsigned char)src[i + 1] & 0xC0) == 0x80) {
        i++;
      }
      dst[j++] = '?';
    } else if (c == '\n' || c == '\r' || c == '\t') {
      dst[j++] = ' ';
    }
  }
  dst[j] = '\0';
}

static int overlay_is_active(void) {
  u32 now = utils_get_time_ms();
  return s_visible_until_ms != 0 && now <= s_visible_until_ms;
}

static void overlay_register_fb(void *vram, int stride, int format) {
  int i;

  if (!vram || stride <= 0) {
    return;
  }

  for (i = 0; i < s_tracked_count; i++) {
    if (s_tracked[i].vram == vram) {
      s_tracked[i].stride = stride;
      s_tracked[i].format = format;
      return;
    }
  }

  if (s_tracked_count < OVERLAY_TRACKED_FB) {
    s_tracked[s_tracked_count].vram = vram;
    s_tracked[s_tracked_count].stride = stride;
    s_tracked[s_tracked_count].format = format;
    s_tracked_count++;
    return;
  }

  /* Ring: overwrite oldest slot */
  s_tracked[0] = s_tracked[1];
  s_tracked[1] = s_tracked[2];
  s_tracked[2] = s_tracked[3];
  s_tracked[3].vram = vram;
  s_tracked[3].stride = stride;
  s_tracked[3].format = format;
}

static void overlay_apply_style(OverlayNotifyStyle style) {
  u32 bg = OVERLAY_COLOR_BG_NEUTRAL;

  switch (style) {
  case OVERLAY_NOTIFY_TRACKER_ON:
    bg = OVERLAY_COLOR_BG_ON;
    break;
  case OVERLAY_NOTIFY_TRACKER_OFF:
    bg = OVERLAY_COLOR_BG_OFF;
    break;
  default:
    break;
  }
  overlay_blit_set_colors(OVERLAY_COLOR_FG, bg);
}

static int s_was_active = 0;

static void overlay_draw_on_hook(void *fb, int bufferwidth, int pixelformat) {
  int active = overlay_is_active() && s_line_count > 0;
  if (active) {
    const char *l1 = (s_line_count >= 1) ? s_lines[0] : NULL;
    const char *l2 = (s_line_count >= 2) ? s_lines[1] : NULL;
    overlay_blit_draw_to(fb, bufferwidth, pixelformat, l1, l2, s_line_count);
    s_was_active = OVERLAY_TRACKED_FB;
  } else if (s_was_active > 0) {
    if (s_line_count > 0) {
      u32 old_fg, old_bg;
      overlay_blit_get_colors(&old_fg, &old_bg);
      /* Erase the overlay by drawing a black box of the exact same size */
      overlay_blit_set_colors(0xFF000000U, 0xFF000000U);
      const char *l1 = (s_line_count >= 1) ? s_lines[0] : NULL;
      const char *l2 = (s_line_count >= 2) ? s_lines[1] : NULL;
      overlay_blit_draw_to(fb, bufferwidth, pixelformat, l1, l2, s_line_count);
      overlay_blit_set_colors(old_fg, old_bg);
    }
    s_was_active--;
  }
}

static int hook_wait_vblank_start(void) {
  int ret = 0;
  if (s_orig_wait_vblank_start) {
    ret = s_orig_wait_vblank_start();
  }
  
  int active = overlay_is_active() && s_line_count > 0;
  if (active) {
    const char *l1 = (s_line_count >= 1) ? s_lines[0] : NULL;
    const char *l2 = (s_line_count >= 2) ? s_lines[1] : NULL;
    overlay_blit_draw_all(l1, l2, s_line_count);
    s_was_active = 10;
  } else if (s_was_active > 0) {
    if (s_line_count > 0) {
      u32 old_fg, old_bg;
      overlay_blit_get_colors(&old_fg, &old_bg);
      /* Clear the overlay area on VBlank by drawing a black box of the exact same size */
      overlay_blit_set_colors(0xFF000000U, 0xFF000000U);
      const char *l1 = (s_line_count >= 1) ? s_lines[0] : NULL;
      const char *l2 = (s_line_count >= 2) ? s_lines[1] : NULL;
      overlay_blit_draw_all(l1, l2, s_line_count);
      overlay_blit_set_colors(old_fg, old_bg);
    }
    s_was_active--;
  }
  
  return ret;
}

static void overlay_on_flip(void *vram, int stride, int format) {
  /* Only track pointers here; draw from tracker thread (avoid display re-entry). */
  overlay_register_fb(vram, stride, format);
}

static int hook_set_frame_buf(void *fb, int bufferwidth, int pixelformat, int sync) {
  int ret = 0;
  if (s_orig_set_frame_buf) {
    ret = s_orig_set_frame_buf(fb, bufferwidth, pixelformat, sync);
  }
  overlay_draw_on_hook(fb, bufferwidth, pixelformat);
  overlay_on_flip(fb, bufferwidth, pixelformat);
  return ret;
}

static int hook_set_frame_buf_internal(int pri, void *fb, int bufferwidth,
                                       int pixelformat, int sync) {
  int ret = 0;
  (void)pri;
  if (s_orig_set_frame_buf_internal) {
    ret = s_orig_set_frame_buf_internal(pri, fb, bufferwidth, pixelformat, sync);
  }
  overlay_draw_on_hook(fb, bufferwidth, pixelformat);
  overlay_on_flip(fb, bufferwidth, pixelformat);
  return ret;
}

static u32 overlay_find_nid(const u32 *nids, int count) {
  int i;
  u32 addr;

  for (i = 0; i < count; i++) {
    addr = sctrlHENFindFunction("sceDisplay_driver", "sceDisplay", nids[i]);
    if (addr) {
      return addr;
    }
    addr = sctrlHENFindFunction("sceDisplay", "sceDisplay", nids[i]);
    if (addr) {
      return addr;
    }
  }
  return 0;
}

static void overlay_install_hooks(void) {
  u32 addr;
  static const u32 setfb_nids[] = {
      NID_SET_FRAME_BUF,
      NID_SET_FRAME_BUF_LEGACY,
  };
  static const u32 internal_nids[] = {
      NID_SET_FRAME_BUF_INTERNAL,
      0x63E22A26U,
  };
  if (s_hooks_installed) {
    return;
  }

  addr = overlay_find_nid(setfb_nids, 2);
  if (addr) {
    sctrlHENHijackFunction(&s_setfb_patch, (void *)addr,
                           (void *)hook_set_frame_buf,
                           (void **)&s_orig_set_frame_buf);
  }

  addr = overlay_find_nid(internal_nids, 2);
  if (addr) {
    sctrlHENHijackFunction(&s_setfb_internal_patch, (void *)addr,
                           (void *)hook_set_frame_buf_internal,
                           (void **)&s_orig_set_frame_buf_internal);
  }

  /* Hook sceDisplayWaitVblankStart (NID: 0x984C27E7) */
  addr = sctrlHENFindFunction("sceDisplay_driver", "sceDisplay", 0x984C27E7U);
  if (addr == 0) {
    addr = sctrlHENFindFunction("sceDisplay", "sceDisplay", 0x984C27E7U);
  }
  if (addr) {
    sctrlHENHijackFunction(&s_vblank_patch, (void *)addr,
                           (void *)hook_wait_vblank_start,
                           (void **)&s_orig_wait_vblank_start);
  }

  s_hooks_installed = 1;
}

void overlay_notification_init(void) {
  s_visible_until_ms = 0;
  s_line_count = 0;
  s_tracked_count = 0;
  memset(s_lines, 0, sizeof(s_lines));
  memset(s_tracked, 0, sizeof(s_tracked));
  overlay_install_hooks();
}

void overlay_notification_shutdown(void) {
  overlay_notification_init();
}

int overlay_notification_is_visible(u32 now_ms) {
  return s_visible_until_ms != 0 && now_ms <= s_visible_until_ms;
}

void overlay_notification_show(const char *line1, const char *line2,
                               OverlayNotifyStyle style) {
  s_line_count = 0;
  memset(s_lines, 0, sizeof(s_lines));

  if (line1 && line1[0]) {
    overlay_sanitize_line(s_lines[0], OVERLAY_LINE_LEN, line1);
    if (s_lines[0][0]) {
      s_line_count = 1;
    }
  }
  if (line2 && line2[0] && s_line_count < OVERLAY_MAX_LINES) {
    overlay_sanitize_line(s_lines[1], OVERLAY_LINE_LEN, line2);
    if (s_lines[1][0]) {
      s_line_count = 2;
    }
  }

  overlay_apply_style(style);
  s_visible_until_ms = utils_get_time_ms() + OVERLAY_HIDE_MS;
}

void overlay_notification_draw(void) {
  int i;
  const char *l1;
  const char *l2;
  void *seen[OVERLAY_TRACKED_FB + 4];
  int nseen = 0;

  if (!overlay_is_active() || s_line_count <= 0) {
    return;
  }

  l1 = (s_line_count >= 1) ? s_lines[0] : NULL;
  l2 = (s_line_count >= 2) ? s_lines[1] : NULL;

  for (i = 0; i < s_tracked_count; i++) {
    void *p = s_tracked[i].vram;
    int j;
    int dup = 0;

    for (j = 0; j < nseen; j++) {
      if (seen[j] == p) {
        dup = 1;
        break;
      }
    }
    if (dup) {
      continue;
    }
    seen[nseen++] = p;
    overlay_blit_draw_to(p, s_tracked[i].stride, s_tracked[i].format, l1, l2,
                         s_line_count);
  }

  overlay_blit_draw_all(l1, l2, s_line_count);
}

int overlay_notification_get_fb_count(void) {
  return s_tracked_count;
}
