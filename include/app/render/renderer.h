#ifndef GAMEDIARY_RENDERER_H
#define GAMEDIARY_RENDERER_H

#include <psptypes.h>
#include <stdint.h>

void renderer_init(void);
void renderer_start_frame(void);
void renderer_end_frame(void);
void renderer_clear(uint32_t color);
void renderer_draw_rect(int x, int y, int w, int h, uint32_t color);

#endif // GAMEDIARY_RENDERER_H
