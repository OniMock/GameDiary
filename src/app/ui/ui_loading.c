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
 * @file ui_loading.c
 * @brief Animated loading popup implementation.
 */

#include "app/ui/ui_loading.h"
#include "app/ui/ui_components.h"
#include "app/render/font.h"
#include "app/render/renderer.h"
#include <pspgu.h>
#include <pspkernel.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * Layout constants
 * ----------------------------------------------------------------------- */

#define SCREEN_W 480
#define SCREEN_H 272

#define LOADING_CARD_H   52

#define SPINNER_RADIUS    10
#define SPINNER_THICKNESS 4.0f

#define FADE_STEP          0.12f

/* Backdrop alpha (60% black) */
#define OVERLAY_MAX_ALPHA  UI_ALPHA(60)

/* -----------------------------------------------------------------------
 * State machine
 * ----------------------------------------------------------------------- */

typedef enum {
    LOADING_STATE_HIDDEN = 0,
    LOADING_STATE_FADE_IN,
    LOADING_STATE_VISIBLE,
    LOADING_STATE_FADE_OUT
} LoadingState;

static LoadingState s_state       = LOADING_STATE_HIDDEN;
static float        s_alpha       = 0.0f;
static int          s_tick        = 0;
static char         s_label[128];

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void ui_loading_show(const char *label) {
    if (label) {
        strncpy(s_label, label, sizeof(s_label) - 1);
        s_label[sizeof(s_label) - 1] = '\0';
    } else {
        s_label[0] = '\0';
    }
    s_tick  = 0;

    if (s_state == LOADING_STATE_HIDDEN || s_state == LOADING_STATE_FADE_OUT) {
        s_state = LOADING_STATE_FADE_IN;
        s_alpha = 0.0f;
    }
}

void ui_loading_hide(void) {
    if (s_state == LOADING_STATE_VISIBLE || s_state == LOADING_STATE_FADE_IN) {
        s_state = LOADING_STATE_FADE_OUT;
    }
}

int ui_loading_is_active(void) {
    return s_state != LOADING_STATE_HIDDEN;
}

void ui_loading_update(void) {
    switch (s_state) {
    case LOADING_STATE_FADE_IN:
        s_alpha += FADE_STEP;
        if (s_alpha >= 1.0f) {
            s_alpha = 1.0f;
            s_state = LOADING_STATE_VISIBLE;
        }
        s_tick++;
        break;

    case LOADING_STATE_VISIBLE:
        s_tick++;
        break;

    case LOADING_STATE_FADE_OUT:
        s_alpha -= FADE_STEP;
        if (s_alpha <= 0.0f) {
            s_alpha  = 0.0f;
            s_state  = LOADING_STATE_HIDDEN;
            s_label[0] = '\0';
        }
        s_tick++;
        break;

    case LOADING_STATE_HIDDEN:
    default:
        break;
    }
}

/* -----------------------------------------------------------------------
 * Presentation layer
 * ----------------------------------------------------------------------- */

static void draw_arc(float cx, float cy, float r, float thickness, float start_angle, float sweep_angle, uint32_t color) {
    if (sweep_angle <= 0.0f) return;

    struct VertexColor {
        uint32_t color;
        float x, y, z;
    };

    int segments = (int)(sweep_angle / 5.0f);
    if (segments < 4) segments = 4;
    int vertex_count = (segments + 1) * 2;

    struct VertexColor *vertices = (struct VertexColor*)sceGuGetMemory(vertex_count * sizeof(struct VertexColor));
    if (!vertices) return;

    float r_in = r - thickness / 2.0f;
    float r_out = r + thickness / 2.0f;

    float start_rad = start_angle * (3.14159265f / 180.0f);
    float sweep_rad = sweep_angle * (3.14159265f / 180.0f);
    float step = sweep_rad / segments;

    for (int i = 0; i <= segments; i++) {
        float angle = start_rad + i * step;
        float c = cosf(angle);
        float s = sinf(angle);

        vertices[i*2].color = color;
        vertices[i*2].x = cx + s * r_in;
        vertices[i*2].y = cy - c * r_in;
        vertices[i*2].z = 0.0f;

        vertices[i*2+1].color = color;
        vertices[i*2+1].x = cx + s * r_out;
        vertices[i*2+1].y = cy - c * r_out;
        vertices[i*2+1].z = 0.0f;
    }

    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuDisable(GU_DEPTH_TEST);

    sceKernelDcacheWritebackRange(vertices, vertex_count * sizeof(struct VertexColor));
    sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, vertex_count, 0, vertices);

    sceGuEnable(GU_TEXTURE_2D);
}

void ui_loading_render(void) {
    if (s_state == LOADING_STATE_HIDDEN) return;

    /* --- 1. Full-screen dim overlay --- */
    uint32_t overlay_alpha = (uint32_t)((float)OVERLAY_MAX_ALPHA * s_alpha);
    renderer_draw_rect(0, 0, SCREEN_W, SCREEN_H, UI_COLOR_ALPHA_VAL(0x000000u, overlay_alpha));

    /* --- 2. Dynamic Card Size Calculation --- */
    float text_w = 0.0f;
    if (s_label[0] != '\0') {
        text_w = font_get_width(s_label, UI_FONT_SIZE_NORMAL);
    }

    float pad_left = 26.0f;
    float spinner_diameter = (SPINNER_RADIUS * 2.0f);
    float gap = 14.0f;
    float pad_right = 26.0f;

    float card_w = pad_left + spinner_diameter + gap + text_w + pad_right;
    if (card_w < 160.0f) card_w = 160.0f;
    if (card_w > (SCREEN_W - 20.0f)) card_w = SCREEN_W - 20.0f;

    int card_x = (SCREEN_W - (int)card_w) / 2;
    int card_y = (SCREEN_H - LOADING_CARD_H) / 2;

    /* --- 3. Card background --- */
    uint32_t card_alpha  = (uint32_t)(UI_ALPHA(90) * s_alpha);
    uint32_t bdr_alpha   = (uint32_t)(UI_ALPHA(31) * s_alpha);

    uint32_t card_bg  = UI_COLOR_ALPHA_VAL(COLOR_CARD, card_alpha);
    uint32_t card_bdr = UI_COLOR_ALPHA_VAL(COLOR_ACCENT, bdr_alpha);

    Rect card = { card_x, card_y, (int)card_w, LOADING_CARD_H };
    ui_draw_card(card, card_bg, card_bdr);

    /* --- 4. Spinner Animation --- */
    float spinner_center_x = card_x + pad_left + SPINNER_RADIUS;
    float spinner_center_y = card_y + (LOADING_CARD_H / 2.0f);

    float cycle = (float)s_tick * 0.06f;
    float sweep = 150.0f + 120.0f * sinf(cycle);
    float start_angle = (float)s_tick * 6.0f;

    uint32_t spinner_alpha = (uint32_t)(255.0f * s_alpha);
    uint32_t spinner_color = UI_COLOR_ALPHA_VAL(COLOR_ACCENT, spinner_alpha);

    draw_arc(spinner_center_x, spinner_center_y, SPINNER_RADIUS, SPINNER_THICKNESS, start_angle, sweep, spinner_color);

    /* --- 5. Label text --- */
    if (s_label[0] != '\0') {
        uint32_t text_alpha = (uint32_t)(255.0f * s_alpha);
        uint32_t text_color = UI_COLOR_ALPHA_VAL(COLOR_TEXT, text_alpha);

        int text_x = (int)(spinner_center_x + SPINNER_RADIUS + gap);
        Rect text_rect = {
            text_x,
            card_y,
            (int)card_w - (text_x - card_x) - (int)pad_right,
            LOADING_CARD_H
        };
        ui_draw_text(s_label, text_rect, text_color, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
    }
}
