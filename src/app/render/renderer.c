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
  * @file renderer.c
  * @brief Renderer system implementation.
  */

#include "app/render/renderer.h"
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <stdio.h>
#include <malloc.h>

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH * SCR_HEIGHT * 2)

static unsigned int __attribute__((aligned(16))) list[262144];

void* g_draw_buffer;
void* g_display_buffer;
void* g_z_buffer;

void renderer_init(void) {
    // Initial display config
    sceDisplaySetMode(0, 480, 272);

    // Standard relative offsets in VRAM for 512-stride 32-bit buffers
    g_draw_buffer = (void*)0;
    g_display_buffer = (void*)0x88000;
    g_z_buffer = (void*)0x110000;

    sceGuInit();

    sceGuStart(GU_DIRECT, list);

    // Setup buffers (relative to VRAM base)
    sceGuDrawBuffer(GU_PSM_8888, g_draw_buffer, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, g_display_buffer, BUF_WIDTH);
    sceGuDepthBuffer(g_z_buffer, BUF_WIDTH);

    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(0xc350, 0x2710);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);

    sceGuEnable(GU_SCISSOR_TEST);
    sceGuAlphaFunc(GU_GREATER, 0, 0xff);
    sceGuEnable(GU_ALPHA_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuEnable(GU_CLIP_PLANES);

    // Finalize init and clear both buffers to avoid startup static
    sceGuClearColor(0xFF000000);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void renderer_start_frame(void) {
    sceGuStart(GU_DIRECT, list);
    // Explicitly set the current draw buffer for the GU
    sceGuDrawBuffer(GU_PSM_8888, g_draw_buffer, BUF_WIDTH);
}

void renderer_end_frame(void) {
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();

    // Swap buffers and get the new draw buffer pointer
    g_display_buffer = g_draw_buffer;
    g_draw_buffer = sceGuSwapBuffers();
}

void renderer_clear(uint32_t color) {
    sceGuClearColor(color);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void renderer_draw_rect(int x, int y, int w, int h, uint32_t color) {
    struct Vertex {
        unsigned int color;
        short x, y, z;
    };

    struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    vertices[0].color = color;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0;

    vertices[1].color = color;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0;

    sceGuDisable(GU_TEXTURE_2D);
    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
    sceGuEnable(GU_TEXTURE_2D);
}
