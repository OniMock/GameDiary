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
  * @file screen_manager.c
  * @brief Screen manager implementation.
  */

#include <pspkernel.h>
#include <pspctrl.h>
#include <psprtc.h>
#include "app/ui/screen.h"
#include "app/ui/ui_popup.h"
#include "app/render/renderer.h"
#include "app/audio/audio_manager.h"

#define MAX_STACK 8

static Screen* g_current_screen = NULL;
static Screen* g_next_screen = NULL;
static Screen* g_screen_stack[MAX_STACK];
static int g_stack_top = -1;

static SceCtrlData s_current_pad;
static uint32_t s_current_pressed;

static u32 g_last_buttons = 0;

static float g_fade_alpha = 0.0f; // 0: fully visible, 1: fully black
static int g_fade_state = 0;     // 0: idle, 1: fading out, 2: fading in

#define FADE_SPEED 6.0f // ~160ms for full transition

void screen_manager_set(Screen* screen) {
    if (screen == NULL || screen == g_current_screen) return;
    g_stack_top = -1; // Clear stack

    if (g_current_screen == NULL) {
        g_current_screen = screen;
        if (g_current_screen->init) g_current_screen->init();
        g_fade_state = 0;
        g_fade_alpha = 0.0f;
        return;
    }

    g_next_screen = screen;
    g_fade_state = 1; // Start fading out
}

void screen_manager_push(Screen* screen) {
    if (screen == NULL || screen == g_current_screen || g_fade_state != 0) return;

    if (g_current_screen != NULL && g_stack_top < MAX_STACK - 1) {
        g_screen_stack[++g_stack_top] = g_current_screen;
    }

    g_next_screen = screen;
    g_fade_state = 1; // Fade out
}

void screen_manager_pop(void) {
    if (g_fade_state != 0 || g_stack_top < 0) return;

    g_next_screen = g_screen_stack[g_stack_top--];
    g_fade_state = 1;
}

void screen_manager_update(void) {
    // 1. Inputs
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);
    u32 pressed = pad.Buttons & ~g_last_buttons;
    g_last_buttons = pad.Buttons;

    // --- POPUP INTERCEPT ---
    if (popup_is_open()) {
        popup_update(pad.Buttons, pressed);
        // Clear input safely so that underlying screens and shortcuts do not act
        pad.Buttons = 0;
        pressed = 0;
        // Neutralize analog stick
        pad.Lx = 128;
        pad.Ly = 128;
    }

    s_current_pad = pad;
    s_current_pressed = pressed;

    // Global Shortcuts
    if (g_fade_state == 0 && g_current_screen != &g_screen_splash) {
        if (pressed & PSP_CTRL_START) {
            audio_play_sfx(SFX_CONFIRM);
            if (g_current_screen != &g_screen_main_menu) {
                screen_manager_push(&g_screen_main_menu);
            }
            return;
        }
        if (pressed & PSP_CTRL_SELECT) {
            audio_play_sfx(SFX_CONFIRM);
            if (g_current_screen != &g_screen_settings) {
                screen_manager_push(&g_screen_settings);
            }
            return;
        }
        if (pressed & PSP_CTRL_CIRCLE) {
            audio_play_sfx(SFX_CANCEL);
            if (g_stack_top >= 0) {
                screen_manager_pop();
                return;
            }
        }
    }

    // 2. State & Transitions
    static int s_fade_wait_frames = 0;

    if (g_fade_state == 1) { // Fading OUT
        g_fade_alpha += (1.0f / 60.0f) * FADE_SPEED;
        if (g_fade_alpha >= 1.0f) {
            g_fade_alpha = 1.0f;
            s_fade_wait_frames = 2; // Guarantee double buffering commits fully black frames to LCD
            g_fade_state = 3; 
        }
    } else if (g_fade_state == 3) {
        if (s_fade_wait_frames > 0) {
            s_fade_wait_frames--;
        } else {
            // The black screen was drawn. Now we can do heavy operations (I/O, parsing) without glitches.
            if (g_current_screen && g_current_screen->destroy) g_current_screen->destroy();
            g_current_screen = g_next_screen;
            if (g_current_screen && g_current_screen->init) g_current_screen->init();

            g_fade_state = 2; // Start fading IN
        }
    } else if (g_fade_state == 2) { // Fading IN
        g_fade_alpha -= (1.0f / 60.0f) * FADE_SPEED;
        if (g_fade_alpha <= 0.0f) {
            g_fade_alpha = 0.0f;
            g_fade_state = 0;
        }
    }

    // 3. Screen Update (only when not in middle of fade transition)
    if (g_current_screen && g_current_screen->update && g_fade_state == 0) {
        g_current_screen->update(pad.Buttons, pressed);
    }
}

void screen_manager_draw(void) {
    /* Optimization & Artifact Prevention: 
     * If the fade has completely enveloped the screen (Pitch Black state), 
     * fully abort drawing the complex UI layers beneath it. 
     * This physically prevents the GE (Graphics Engine) from tearing or 
     * flashing stale buffers / old geometry when switching the heavy screens. */
    if (g_fade_alpha >= 1.0f || g_fade_state == 3) {
        renderer_clear(0xFF000000); // Pure Pitch Black
        return;
    }

    if (g_current_screen && g_current_screen->draw) {
        g_current_screen->draw();
    }

    // Draw fade overlay
    if (g_fade_alpha > 0.0f) {
        u32 alpha = (u32)(g_fade_alpha * 255.0f);
        if (alpha > 255) alpha = 255;
        renderer_draw_rect(0, 0, 480, 272, (alpha << 24)); // Black with fade alpha
    }

    if (popup_is_open()) {
        popup_render();
    }
}

const SceCtrlData* ui_get_pad(void) {
    return &s_current_pad;
}

uint32_t ui_get_pressed(void) {
    return s_current_pressed;
}
