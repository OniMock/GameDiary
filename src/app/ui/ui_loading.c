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
 *
 * Visual design:
 *   +-----------------------------------------+
 *   |              (dim black overlay)         |
 *   |                                          |
 *   |   +-----------------------------+        |
 *   |   |   [spinner]  Loading...     |        |
 *   |   +-----------------------------+        |
 *   |                                          |
 *   +-----------------------------------------+
 *
 * The spinner is a segmented arc drawn via filled rectangles arranged in
 * a clock pattern (8 segments). Each frame the leading segment advances,
 * giving the classic "activity indicator" look without needing a texture.
 *
 * Segment brightness fades from the leading edge (255) to the tail (32),
 * so the animation reads as a rotating glow.
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
 * Layout constants — adjust here, not in every render call
 * ----------------------------------------------------------------------- */

/* PSP screen dimensions */
#define SCREEN_W 480
#define SCREEN_H 272

/* Popup card size */
#define LOADING_CARD_W  200
#define LOADING_CARD_H   52

/* Derived: card top-left so it sits exactly at screen center */
#define LOADING_CARD_X  ((SCREEN_W - LOADING_CARD_W) / 2)
#define LOADING_CARD_Y  ((SCREEN_H - LOADING_CARD_H) / 2)

/* Spinner geometry (8-segment activity indicator) */
#define SPINNER_SEGMENTS   8
#define SPINNER_RADIUS    10   /* distance from wheel center to segment center */
#define SPINNER_SEG_W      4   /* segment rect width  */
#define SPINNER_SEG_H      8   /* segment rect height */
#define SPINNER_CENTER_X  (LOADING_CARD_X + 26)   /* wheel pivot X */
#define SPINNER_CENTER_Y  (LOADING_CARD_Y + LOADING_CARD_H / 2)

/* Text position: to the right of the spinner */
#define SPINNER_TEXT_X    (SPINNER_CENTER_X + SPINNER_RADIUS + 10)
#define SPINNER_TEXT_Y    LOADING_CARD_Y

/* Animation */
#define SPINNER_TICK_RATE  4   /* frames per segment advance             */
#define FADE_STEP          0.12f /* alpha delta per frame during fade in/out */

/* Backdrop alpha (10% black = 25 / 255) */
#define OVERLAY_MAX_ALPHA  25u

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
static float        s_alpha       = 0.0f;  /* 0.0 … 1.0 — drives all fades     */
static int          s_tick        = 0;     /* frames since last segment advance  */
static int          s_lead_seg    = 0;     /* index of the "brightest" segment   */
static const char  *s_label       = NULL;  /* caller-owned string pointer        */

/* -----------------------------------------------------------------------
 * Public API — Domain / Application layer
 * ----------------------------------------------------------------------- */

void ui_loading_show(const char *label) {
    /* Allow re-showing with a new label while open (e.g. "Verifying...") */
    s_label = label;
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
        break;

    case LOADING_STATE_VISIBLE:
        /* Advance spinner segment every SPINNER_TICK_RATE frames */
        s_tick++;
        if (s_tick >= SPINNER_TICK_RATE) {
            s_tick = 0;
            s_lead_seg = (s_lead_seg + 1) % SPINNER_SEGMENTS;
        }
        break;

    case LOADING_STATE_FADE_OUT:
        s_alpha -= FADE_STEP;
        if (s_alpha <= 0.0f) {
            s_alpha  = 0.0f;
            s_state  = LOADING_STATE_HIDDEN;
            s_label  = NULL;
        }
        break;

    case LOADING_STATE_HIDDEN:
    default:
        break;
    }
}

/* -----------------------------------------------------------------------
 * Presentation layer — pure drawing, zero state changes
 * ----------------------------------------------------------------------- */

/**
 * draw_segment — draws one spinner tick mark.
 *
 * Each segment is a small rectangle placed at angle `angle_deg` around
 * the spinner center at distance SPINNER_RADIUS.
 *
 * We rotate the rect position via sin/cos, then draw it axis-aligned
 * (no true rotation — the PSP GE doesn't do per-sprite rotation in 2-D
 * mode without a matrix). An axis-aligned rect still reads visually as a
 * rotating dial for this size (4×8 px) at normal viewing distance.
 */
static void draw_segment(int cx, int cy, float angle_deg, uint8_t brightness) {
    float rad = angle_deg * (3.14159265f / 180.0f);

    /* Segment center offset from the wheel center */
    int sx = cx + (int)(sinf(rad) * (float)SPINNER_RADIUS);
    int sy = cy - (int)(cosf(rad) * (float)SPINNER_RADIUS);

    /* Top-left of the rect (centred on sx, sy) */
    int rx = sx - SPINNER_SEG_W / 2;
    int ry = sy - SPINNER_SEG_H / 2;

    uint32_t color = ((uint32_t)brightness << 24) | (COLOR_ACCENT & 0x00FFFFFFu);
    renderer_draw_rect(rx, ry, SPINNER_SEG_W, SPINNER_SEG_H, color);
}

void ui_loading_render(void) {
    if (s_state == LOADING_STATE_HIDDEN) return;

    /* --- 1. Full-screen dim overlay (10% black) --- */
    uint32_t overlay_alpha = (uint32_t)((float)OVERLAY_MAX_ALPHA * s_alpha);
    renderer_draw_rect(0, 0, SCREEN_W, SCREEN_H,
                       (overlay_alpha << 24) | 0x000000u);

    /* --- 2. Card background --- */
    uint32_t card_alpha  = (uint32_t)(230.0f * s_alpha);  /* ~90% opaque */
    uint32_t bdr_alpha   = (uint32_t)(80.0f  * s_alpha);

    uint32_t card_bg  = (card_alpha << 24) | (0x1A1A2E & 0x00FFFFFFu); /* deep navy */
    uint32_t card_bdr = (bdr_alpha  << 24) | (COLOR_ACCENT & 0x00FFFFFFu);

    Rect card = { LOADING_CARD_X, LOADING_CARD_Y, LOADING_CARD_W, LOADING_CARD_H };
    ui_draw_card(card, card_bg, card_bdr);

    /* --- 3. Spinner --- */
    /* Angle step between adjacent segments: 360 / 8 = 45° */
    const float angle_step = 360.0f / (float)SPINNER_SEGMENTS;

    for (int i = 0; i < SPINNER_SEGMENTS; i++) {
        /*
         * Distance from the leading segment (going backwards).
         * dist == 0 → brightest (lead), dist == SEGMENTS-1 → dimmest (tail).
         */
        int dist = (s_lead_seg - i + SPINNER_SEGMENTS) % SPINNER_SEGMENTS;

        /*
         * Brightness ramps from 255 (lead) down to 32 (tail).
         * Linear: brightness = 255 - dist * step
         */
        uint8_t brightness = (uint8_t)(255 - dist * ((255 - 32) / (SPINNER_SEGMENTS - 1)));

        /* Attenuate by the overall fade alpha */
        brightness = (uint8_t)(((uint32_t)brightness * (uint32_t)(s_alpha * 255.0f)) / 255u);

        float angle = (float)i * angle_step;
        draw_segment(SPINNER_CENTER_X, SPINNER_CENTER_Y, angle, brightness);
    }

    /* --- 4. Label text --- */
    if (s_label) {
        uint32_t text_alpha = (uint32_t)(255.0f * s_alpha);
        uint32_t text_color = (text_alpha << 24) | (COLOR_TEXT & 0x00FFFFFFu);

        Rect text_rect = {
            SPINNER_TEXT_X,
            SPINNER_TEXT_Y,
            LOADING_CARD_W - (SPINNER_TEXT_X - LOADING_CARD_X) - 8,
            LOADING_CARD_H
        };
        ui_draw_text(s_label, text_rect, text_color, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
    }
}
