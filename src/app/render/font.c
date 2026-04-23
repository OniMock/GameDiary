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

/*
 * IntraFont used a scale factor where 1.0 ≈ 10px on screen.
 * Our SDF renderer now takes explicit pixel height from the UI layer.
 */
#define INTRAFONT_SCALE_TO_PX(s) (s)

int font_init(void) {
    return sdf_font_init();
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

void font_cleanup(void) {
    sdf_font_cleanup();
}

