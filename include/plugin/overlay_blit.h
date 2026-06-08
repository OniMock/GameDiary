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
 * @file overlay_blit.h
 * @brief Overlay blit implementation.
 */

#ifndef GAMEDIARY_OVERLAY_BLIT_H
#define GAMEDIARY_OVERLAY_BLIT_H

#include <psptypes.h>

/** @brief Foreground/background colors for the next draw (ARGB 8888). */
void overlay_blit_set_colors(u32 fg_argb, u32 bg_argb);
void overlay_blit_get_colors(u32 *fg_argb, u32 *bg_argb);

/** Draw up to two lines on every framebuffer we can resolve (universal HUD path). */
void overlay_blit_draw_all(const char *line1, const char *line2, int line_count);

/** Draw on a specific VRAM pointer (used from display hooks). */
void overlay_blit_draw_to(void *vram, int bufferwidth, int pixelformat,
                          const char *line1, const char *line2, int line_count);

/** @brief Also try standard EDRAM bases (homebrews that skip SetFrameBuf). */
void overlay_blit_draw_fallbacks(const char *line1, const char *line2, int line_count);

#endif /* GAMEDIARY_OVERLAY_BLIT_H */
