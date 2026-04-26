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
  * @file font.c
  * @brief Font system implementation.
  */

#include "app/render/font.h"
#include "app/render/sdf_font.h"
#include <pspkernel.h>
#include <pspgu.h>
#include <stdio.h>
#include <intraFont.h>

static intraFont *g_fallback_font = NULL;

/*
 * IntraFont used a scale factor where 1.0 ≈ 10px on screen.
 * Our SDF renderer now takes explicit pixel height from the UI layer.
 */
#define INTRAFONT_SCALE_TO_PX(s) ((s) / 22.0f)

int font_init(void) {
    int res = sdf_font_init();

    intraFontInit();
    /* 'INTRAFONT_CACHE_LARGE' is practically mandatory for Japanese glyphs
     * to avoid overflowing the standard 256x256 cache. */
    g_fallback_font = intraFontLoad("flash0:/font/jpn0.pgf", INTRAFONT_CACHE_LARGE | INTRAFONT_STRING_UTF8);

    return res;
}

void font_draw_string(float x, float y, const char *str, uint32_t color, float size) {
    sdf_font_draw_string(x, y, str, color, size);
}

void font_draw_string_centered(float x, float y, const char *str, uint32_t color, float size) {
    sdf_font_draw_string_centered(x, y, str, color, size);
}

float font_get_width(const char *str, float size) {
    return sdf_font_get_width(str, size);
}

float font_get_height(float size) {
    return size;
}

void font_draw_game_name(float x, float y, const char *str, uint32_t color, float size, int align) {
    if (!str || !*str) return;

    if (sdf_font_has_missing_glyphs(str)) {
        if (g_fallback_font) {
            float scale = INTRAFONT_SCALE_TO_PX(size);
            int intra_align = INTRAFONT_ALIGN_LEFT;
            if (align == 1) intra_align = INTRAFONT_ALIGN_CENTER;
            else if (align == 2) intra_align = INTRAFONT_ALIGN_RIGHT;

            intraFontSetStyle(g_fallback_font, scale, color, 0, 0.0f, intra_align | INTRAFONT_STRING_UTF8);
            intraFontPrint(g_fallback_font, x, y, str);
        } else {
            /* DEBUG: If the font failed to load entirely, we draw an error visually! */
            if (align == 1) {
                sdf_font_draw_string_centered(x, y, "[N/FONT]", 0xFF0000FF, size);
            } else if (align == 2) {
                float w = sdf_font_get_width("[N/FONT]", size);
                sdf_font_draw_string(x - w, y, "[N/FONT]", 0xFF0000FF, size);
            } else {
                sdf_font_draw_string(x, y, "[N/FONT]", 0xFF0000FF, size);
            }
        }
    } else {
        if (align == 1) {
            sdf_font_draw_string_centered(x, y, str, color, size);
        } else if (align == 2) {
            float w = sdf_font_get_width(str, size);
            sdf_font_draw_string(x - w, y, str, color, size);
        } else {
            sdf_font_draw_string(x, y, str, color, size);
        }
    }
}

float font_get_game_name_width(const char *str, float size) {
    if (!str || !*str) return 0.0f;

    if (g_fallback_font && sdf_font_has_missing_glyphs(str)) {
        float scale = INTRAFONT_SCALE_TO_PX(size);
        intraFontSetStyle(g_fallback_font, scale, 0xFFFFFFFF, 0, 0.0f, INTRAFONT_ALIGN_LEFT | INTRAFONT_STRING_UTF8);
        return intraFontMeasureText(g_fallback_font, str);
    } else {
        return sdf_font_get_width(str, size);
    }
}

void font_cleanup(void) {
    sdf_font_cleanup();
    if (g_fallback_font) {
        intraFontUnload(g_fallback_font);
        g_fallback_font = NULL;
    }
    intraFontShutdown();
}

