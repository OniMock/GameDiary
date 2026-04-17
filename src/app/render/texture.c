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
  * @file texture.c
  * @brief Texture system implementation.
  */

#include "app/render/texture.h"
#include <pspkernel.h>
#include <pspgu.h>
#include <png.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>


static int get_next_power_of_two(int n) {
    int x = 1;
    while (x < n) x <<= 1;
    return x;
}

Texture* texture_load_png(const char* filename) {
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    FILE *fp;

    if ((fp = fopen(filename, "rb")) == NULL) return NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sig_read);
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);

    Texture* texture = (Texture*)malloc(sizeof(Texture));
    texture->width = width;
    texture->height = height;
    texture->textureWidth = get_next_power_of_two(width);
    texture->textureHeight = get_next_power_of_two(height);

    texture->data = memalign(16, texture->textureWidth * texture->textureHeight * 4);

    int channels = png_get_channels(png_ptr, info_ptr);

    png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

    uint32_t* data_ptr = (uint32_t*)texture->data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            png_bytep row = row_pointers[y];
            png_bytep ptr = &(row[x * channels]);
            uint8_t r = ptr[0];
            uint8_t g = (channels > 1) ? ptr[1] : r;
            uint8_t b = (channels > 2) ? ptr[2] : g;
            uint8_t a = (channels > 3) ? ptr[3] : 255;
            data_ptr[y * texture->textureWidth + x] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    sceKernelDcacheWritebackAll();
    return texture;
}

void texture_draw(Texture* tex, int x, int y, int w, int h) {
    if (!tex) return;

    struct Vertex {
        float u, v;
        unsigned int color;
        float x, y, z;
    };

    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, tex->textureWidth, tex->textureHeight, tex->textureWidth, tex->data);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);

    struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));

    vertices[0].u = 0;
    vertices[0].v = 0;
    vertices[0].color = 0xFFFFFFFF;
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].z = 0;

    vertices[1].u = tex->width;
    vertices[1].v = tex->height;
    vertices[1].color = 0xFFFFFFFF;
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].z = 0;

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, vertices);
}

void texture_draw_resource(const ImageResource* res, int x, int y, int w, int h) {
    if (!res || !res->data) return;

    typedef struct {
        float u, v;
        float x, y, z;
    } Vertex;

    // Flush cache to ensure GPU sees the correct data
    sceKernelDcacheWritebackRange(res->data, res->size);

    sceGuEnable(GU_TEXTURE_2D);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuTexMode(res->format, 0, 0, GU_FALSE);
    sceGuTexImage(0, res->pot_width, res->pot_height, res->stride, res->data);

    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); // Modulate to support tinting via sceGuColor

    Vertex* vertices = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));

    // UV coordinates based on actual dimensions, not stride
    float u = (float)res->width;
    float v = (float)res->height;

    vertices[0] = (Vertex){0, 0, (float)x, (float)y, 0};
    vertices[1] = (Vertex){u, v, (float)(x + w), (float)(y + h), 0};

    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, vertices);
}

void texture_free(Texture* tex) {
    if (tex) {
        if (tex->data) free(tex->data);
        free(tex);
    }
}

void texture_draw_tinted(Texture* tex, int x, int y, int w, int h, u32 color) {
    if (!tex) return;

    /* Same vertex format as texture_draw but with caller-supplied tint color.
     * Using GU_TFX_MODULATE: final_color = texture_color * vertex_color.
     * Pass 0xFFFFFFFF for no tint (identity modulation).               */
    struct Vertex {
        float        u, v;
        unsigned int color;
        float        x, y, z;
    };

    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, tex->textureWidth, tex->textureHeight, tex->textureWidth,
                  tex->data);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);

    struct Vertex* v = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
    v[0].u = 0;           v[0].v = 0;
    v[0].color = color;   v[0].x = (float)x;       v[0].y = (float)y;       v[0].z = 0;
    v[1].u = tex->width;  v[1].v = tex->height;
    v[1].color = color;   v[1].x = (float)(x + w); v[1].y = (float)(y + h); v[1].z = 0;

    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF |
                   GU_TRANSFORM_2D,
                   2, 0, v);
}
