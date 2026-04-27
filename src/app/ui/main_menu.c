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
  * @file main_menu.c
  * @brief Horizontal Carousel Main Menu screen.
  */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/audio/audio_manager.h"
#include <pspctrl.h>
#include <math.h>
#include <stdio.h>

#define MENU_ITEM_COUNT 4

static float g_current_index = 0.0f;
static float g_target_index = 0.0f;

static const char* s_helper_lines[8];
static PopupData s_helper_data;

typedef struct {
    MessageId label_msg;
    const ImageResource* icon;
    Screen* target_screen;
} MainMenuItem;

static const MainMenuItem g_menu_items[MENU_ITEM_COUNT] = {
    { MSG_MENU_GAMES, &GD_IMG_ICON_GAME_128_PNG, &g_screen_game_list },
    { MSG_MENU_STATS, &GD_IMG_ICON_STATS_128_PNG, &g_screen_stats },
    { MSG_MENU_ACTIVITY, &GD_IMG_ICON_ACTIVITY_128_PNG, &g_screen_activity },
    { MSG_MENU_SETTINGS, &GD_IMG_ICON_SETTINGS_128_PNG, &g_screen_settings }
};

static void main_menu_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_SELECT);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_ANALOG_NAVIGATE);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_SELECT_CONFIG);
    s_helper_lines[5] = "";
    s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_MAIN_MENU);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;

    // Preserve target_index between visits, snap current_index for visual entry
    g_current_index = g_target_index;
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

static void main_menu_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    static int repeat_timer = 0;
    static int last_dir = 0;

    const SceCtrlData* pad_ptr = ui_get_pad();

    // 1. Detect Direction (Discrete & Analog)
    float ax = (pad_ptr->Lx - 128.0f) / 128.0f;
    int current_dir = 0;
    if ((buttons & PSP_CTRL_RIGHT) || (ax > 0.5f)) current_dir = 1;
    else if ((buttons & PSP_CTRL_LEFT) || (ax < -0.5f)) current_dir = -1;

    int move = 0;
    if (current_dir != 0) {
        if (current_dir != last_dir) {
            // New direction or initial press: move immediately
            move = current_dir;
            repeat_timer = 25; // Initial delay before repeating (approx 400ms)
        } else {
            // Holding same direction: handle repetition
            repeat_timer--;
            if (repeat_timer <= 0) {
                move = current_dir;
                repeat_timer = 6; // Fast repeat delay (approx 100ms)
            }
        }
    } else {
        repeat_timer = 0;
    }
    last_dir = current_dir;

    if (move != 0) {
        float prev_target = g_target_index;
        g_target_index += move;
        // Clamp to valid range
        if (g_target_index < 0) g_target_index = 0;
        if (g_target_index >= MENU_ITEM_COUNT) g_target_index = MENU_ITEM_COUNT - 1;
        
        if (g_target_index != prev_target) {
            audio_play_sfx(SFX_NAVIGATE);
        }
    }

    // Lerp
    g_current_index += (g_target_index - g_current_index) * 0.15f;

    // Selection
    if (pressed & PSP_CTRL_CROSS) {
        int idx = (int)(g_target_index + 0.5f);
        if (idx >= 0 && idx < MENU_ITEM_COUNT) {
            audio_play_sfx(SFX_CONFIRM);
            screen_manager_push(g_menu_items[idx].target_screen);
        }
    }
}

static void main_menu_draw(void) {
    renderer_clear(COLOR_BG);

    // Header Title
    Rect safe_rect = {20, 20, 440, 232};
    ui_draw_app_header(safe_rect);

    int center_x = 240;
    int center_y = 136;
    int spacing = 140;

    // Draw up to 5 items (center, left 2, right 2)
    // We sort them by absolute offset (depth) to render those furthest away first

    int draw_order[5];
    float offsets[5];
    int count = 0;

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        float offset = (float)i - g_current_index;
        if (fabsf(offset) <= 2.5f) {
            offsets[count] = offset;
            draw_order[count] = i;
            count++;
        }
    }

    // Sort by fabsf(offset) descending
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (fabsf(offsets[i]) < fabsf(offsets[j])) {
                float tmp_off = offsets[i];
                offsets[i] = offsets[j];
                offsets[j] = tmp_off;
                int tmp_idx = draw_order[i];
                draw_order[i] = draw_order[j];
                draw_order[j] = tmp_idx;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        int idx = draw_order[i];
        float offset = offsets[i];

        float scale = 1.0f - fabsf(offset) * 0.2f;
        if (scale < 0.6f) scale = 0.6f;

        float alpha_f = 1.0f - fabsf(offset) * 0.3f;
        if (alpha_f < 0.2f) alpha_f = 0.2f;

        int draw_x = center_x + (int)(offset * spacing);

        // Base sizes for icons (e.g. 64x64)
        int base_w = 64, base_h = 64;
        if (g_menu_items[idx].icon != NULL) {
            base_w = g_menu_items[idx].icon->width;
            base_h = g_menu_items[idx].icon->height;
            // Since some are small (24x24), we should scale them up baseline. Let's make base size 80
            base_w = 80;
            base_h = 80;
        }

        int w = (int)(base_w * scale);
        int h = (int)(base_h * scale);
        int x = draw_x - w / 2;
        int y = center_y - h / 2 - 10;

        u8 a = (u8)(alpha_f * 255.0f);
        u32 tint = ((u32)a << 24) | 0x00FFFFFFu;

        if (g_menu_items[idx].icon) {
            // 1. Draw Shadow (Black tint, offset 2px)
            u32 shadow_tint = ((u32)a << 24) | 0x00000000u;
            sceGuColor(shadow_tint);
            texture_draw_resource(g_menu_items[idx].icon, x + 2, y + 2, w, h);

            // 2. Draw Main Icon (White tint, original position)
            sceGuColor(tint); 
            texture_draw_resource(g_menu_items[idx].icon, x, y, w, h);
            
            sceGuColor(0xFFFFFFFF); // Reset
        }

        // Draw label if it's the center item
        if (fabsf(offset) < 0.1f) {
            float text_alpha = 1.0f - (fabsf(offset) * 10.0f); // fade text quickly
            if (text_alpha > 0.0f) {
                u32 text_col = COLOR_TEXT; // We don't have alpha easily passing into ui_draw_text without modifications, but we can assume fully opaque for near-center
                ui_draw_text(i18n_get(g_menu_items[idx].label_msg), (Rect){draw_x - 100, center_y + 45, 200, 20}, text_col, UI_FONT_SIZE_TITLE_HUGE, ALIGN_CENTER);
            }
        }
    }

    // Standardized hints
    ui_draw_standard_hints();
}

Screen g_screen_main_menu = {
    main_menu_init,
    main_menu_update,
    main_menu_draw,
    NULL
};
