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

#define FADE_DURATION_MS 500
#define HOLD_DURATION_MS 1000

typedef enum {
    SPLASH_STATE_FADE_IN,
    SPLASH_STATE_LOAD,
    SPLASH_STATE_HOLD,
    SPLASH_STATE_FADE_OUT,
    SPLASH_STATE_DONE
} SplashState;

static SplashState s_state;
static u64 s_start_time;
static u64 s_state_start_time;
static u8 s_alpha;

static u64 get_time_ms(void) {
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return tick / (sceRtcGetTickResolution() / 1000);
}

static void splash_init(void) {
    s_state = SPLASH_STATE_FADE_IN;
    s_start_time = get_time_ms();
    s_state_start_time = s_start_time;
    s_alpha = 0;
}

bool splash_is_loading(void) {
    return s_state == SPLASH_STATE_LOAD;
}

void splash_do_load_tasks(void) {
    config_load();
    
    char base_path[128];
    snprintf(base_path, sizeof(base_path), "%s%s", utils_get_device_prefix(), GDIARY_BASE_DIR);
    storage_init(base_path);

    font_init();
    i18n_init(config_get()->language);
    audio_init();
    data_load_all();
    
    s_state = SPLASH_STATE_HOLD;
    /* Reset the holding time so we still see the logo briefly if loading was somewhat fast */
    s_state_start_time = get_time_ms();
}

static void splash_update(u32 buttons, u32 pressed) {
    (void)buttons; (void)pressed;
    u64 now = get_time_ms();
    u64 elapsed = now - s_state_start_time;

    switch (s_state) {
        case SPLASH_STATE_FADE_IN:
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

        case SPLASH_STATE_HOLD:
            if (elapsed >= HOLD_DURATION_MS) {
                s_state = SPLASH_STATE_FADE_OUT;
                s_state_start_time = now;
            }
            break;

        case SPLASH_STATE_FADE_OUT:
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

    int img_w = GD_IMG_ICON_SPLASH_PNG.width;
    int img_h = GD_IMG_ICON_SPLASH_PNG.height;
    int x = (480 - img_w) / 2;
    int y = (272 - img_h) / 2;

    u32 color = (s_alpha << 24) | 0x00FFFFFF;

    texture_draw_resource_tinted(&GD_IMG_ICON_SPLASH_PNG, x, y, img_w, img_h, color);
}

static void splash_destroy(void) {
}

Screen g_screen_splash = {
    splash_init,
    splash_update,
    splash_draw,
    splash_destroy
};
