/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/ui/splash.h"
#include "app/render/renderer.h"
#include "app/render/texture.h"
#include "app/render/image_resources.h"
#include "app/ui/ui_style.h"
#include "app/data/data_loader.h"
#include "app/render/font.h"
#include "app/i18n/i18n.h"
#include "app/config/config.h"
#include "app/audio/audio_manager.h"
#include "common/utils.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include <stdio.h>
#include <psptypes.h>
#include <psprtc.h>
#include <pspkernel.h>

#define FADE_DURATION_MS 400
#define HOLD_DURATION_MS 800

typedef enum {
    SPLASH_STATE_FADE_IN_1,
    SPLASH_STATE_LOAD,
    SPLASH_STATE_HOLD_1,
    SPLASH_STATE_FADE_OUT_1,
    SPLASH_STATE_FADE_IN_2,
    SPLASH_STATE_HOLD_2,
    SPLASH_STATE_FADE_OUT_2,
    SPLASH_STATE_DONE
} SplashState;

static SplashState s_state;
static u64 s_start_time;
static u64 s_state_start_time;
static u8 s_alpha;
static const ImageResource* s_current_image;

static void splash_init(void) {
    s_state = SPLASH_STATE_FADE_IN_1;
    s_start_time = utils_get_time_ms();
    s_state_start_time = s_start_time;
    s_alpha = 0;
    s_current_image = &GD_IMG_ICON_SPLASH_PNG;
}

bool splash_is_loading(void) {
    return s_state == SPLASH_STATE_LOAD;
}

void splash_do_load_tasks(void) {
    config_load();

    if (config_get()->theme == 1) {
        ui_style_set_light();
    } else {
        ui_style_set_dark();
    }

    char base_path[128];
    snprintf(base_path, sizeof(base_path), "%s%s", utils_get_device_prefix(), GDIARY_BASE_DIR);
    storage_init(base_path);

    font_init();
    i18n_init(config_get()->language);
    audio_init();
    data_load_all();

    s_state = SPLASH_STATE_HOLD_1;
    /* Reset the holding time so we still see the logo briefly if loading was somewhat fast */
    s_state_start_time = utils_get_time_ms();
}

static void splash_update(u32 buttons, u32 pressed) {
    (void)buttons; (void)pressed;
    u64 now = utils_get_time_ms();
    u64 elapsed = now - s_state_start_time;

    switch (s_state) {
        case SPLASH_STATE_FADE_IN_1:
            if (elapsed >= FADE_DURATION_MS) {
                s_alpha = 255;
                s_state = SPLASH_STATE_LOAD;
            } else {
                s_alpha = (u8)((elapsed * 255) / FADE_DURATION_MS);
            }
            break;

        case SPLASH_STATE_LOAD:
            /* Main thread will pick this up outside of renderer frame */
            break;

        case SPLASH_STATE_HOLD_1:
            if (elapsed >= HOLD_DURATION_MS) {
                s_state = SPLASH_STATE_FADE_OUT_1;
                s_state_start_time = now;
            }
            break;

        case SPLASH_STATE_FADE_OUT_1:
            if (elapsed >= FADE_DURATION_MS) {
                s_alpha = 0;
                s_state = SPLASH_STATE_FADE_IN_2;
                s_state_start_time = now;
                s_current_image = &GD_IMG_ICON_SPLASH_DEVELOPED_PNG;
            } else {
                s_alpha = (u8)(255 - ((elapsed * 255) / FADE_DURATION_MS));
            }
            break;

        case SPLASH_STATE_FADE_IN_2:
            if (elapsed >= FADE_DURATION_MS) {
                s_alpha = 255;
                s_state = SPLASH_STATE_HOLD_2;
                s_state_start_time = now;
            } else {
                s_alpha = (u8)((elapsed * 255) / FADE_DURATION_MS);
            }
            break;

        case SPLASH_STATE_HOLD_2:
            if (elapsed >= HOLD_DURATION_MS) {
                s_state = SPLASH_STATE_FADE_OUT_2;
                s_state_start_time = now;
            }
            break;

        case SPLASH_STATE_FADE_OUT_2:
            if (elapsed >= FADE_DURATION_MS) {
                s_alpha = 0;
                s_state = SPLASH_STATE_DONE;
                screen_manager_set(&g_screen_main_menu);
            } else {
                s_alpha = (u8)(255 - ((elapsed * 255) / FADE_DURATION_MS));
            }
            break;

        case SPLASH_STATE_DONE:
            // Waiting for screen manager to switch
            break;
    }
}

static void splash_draw(void) {
    renderer_clear(0xFF000000); // Clear to Black

    if (!s_current_image) return;

    int img_w = s_current_image->width;
    int img_h = s_current_image->height;
    int x = (480 - img_w) / 2;
    int y = (272 - img_h) / 2;

    u32 color = UI_COLOR_ALPHA_VAL(0xFFFFFFFF, s_alpha);

    texture_draw_resource_tinted(s_current_image, x, y, img_w, img_h, color);
}

static void splash_destroy(void) {
}

Screen g_screen_splash = {
    splash_init,
    splash_update,
    splash_draw,
    splash_destroy
};
