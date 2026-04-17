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
#include "app/render/renderer.h"

static Screen* g_current_screen = NULL;
static Screen* g_next_screen = NULL;
static u32 g_last_buttons = 0;

static float g_fade_alpha = 0.0f; // 0: fully visible, 1: fully black
static int g_fade_state = 0;     // 0: idle, 1: fading out, 2: fading in

#define FADE_SPEED 6.0f // ~160ms for full transition

void screen_manager_set(Screen* screen) {
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

void screen_manager_update(void) {
    // 1. Inputs
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);
    u32 pressed = pad.Buttons & ~g_last_buttons;
    g_last_buttons = pad.Buttons;

    // 2. State & Transitions
    if (g_fade_state == 1) { // Fading OUT
        g_fade_alpha += (1.0f / 60.0f) * FADE_SPEED;
        if (g_fade_alpha >= 1.0f) {
            g_fade_alpha = 1.0f;

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

    // 3. Screen Update (only when not in middle of fade out)
    if (g_current_screen && g_current_screen->update && g_fade_state != 1) {
        g_current_screen->update(pad.Buttons, pressed);
    }
}

void screen_manager_draw(void) {
    if (g_current_screen && g_current_screen->draw) {
        g_current_screen->draw();
    }

    // Draw fade overlay
    if (g_fade_alpha > 0.0f) {
        u32 alpha = (u32)(g_fade_alpha * 255.0f);
        if (alpha > 255) alpha = 255;
        renderer_draw_rect(0, 0, 480, 272, (alpha << 24)); // Black with fade alpha
    }
}
