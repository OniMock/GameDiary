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

/** Draw up to two lines on every framebuffer we can resolve (universal HUD path). */
void overlay_blit_draw_all(const char *line1, const char *line2, int line_count);

/** Draw on a specific VRAM pointer (used from display hooks). */
void overlay_blit_draw_to(void *vram, int bufferwidth, int pixelformat,
                          const char *line1, const char *line2, int line_count);

#endif /* GAMEDIARY_OVERLAY_BLIT_H */
