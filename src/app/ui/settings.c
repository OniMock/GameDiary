#include "app/ui/screen.h"
#include "app/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include <pspctrl.h>
#include <string.h>
#include <stdio.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>

#define MAX_LANGS 16
static char g_lang_list[MAX_LANGS][8];
static int g_lang_count = 0;
static int g_selection = 0;

static void list_langs(void) {
    g_lang_count = 0;
    SceUID dfd = sceIoDopen("ms0:/PSP/COMMON/GameDiary/strings");
    if (dfd >= 0) {
        SceIoDirent entry;
        while (sceIoDread(dfd, &entry) > 0 && g_lang_count < MAX_LANGS) {
            if (!FIO_S_ISDIR(entry.d_stat.st_mode)) {
                char *ext = strrchr(entry.d_name, '.');
                if (ext && strcmp(ext, ".lang") == 0) {
                    *ext = 0;
                    strncpy(g_lang_list[g_lang_count++], entry.d_name, 7);
                }
            }
        }
        sceIoDclose(dfd);
    }
}

static void settings_init(void) {
    list_langs();
    const char* current = i18n_current_lang();
    for (int i = 0; i < g_lang_count; i++) {
        if (strcmp(g_lang_list[i], current) == 0) {
            g_selection = i;
            break;
        }
    }
}

static void settings_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & PSP_CTRL_UP) {
        g_selection--;
        if (g_selection < 0) g_selection = g_lang_count - 1;
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection++;
        if (g_selection >= g_lang_count) g_selection = 0;
    }
    if (pressed & PSP_CTRL_CROSS) {
        // Apply language
        if (i18n_load(g_lang_list[g_selection]) == 0) {
            strncpy(config_get()->language, g_lang_list[g_selection], 7);
            config_save();
        }
    }
    if (pressed & PSP_CTRL_CIRCLE) {
        screen_manager_set(&g_screen_dashboard);
    }
}

static void settings_draw(void) {
    renderer_clear(0xFF222222);
    
    font_draw_string_centered(240, 40, i18n_get("menu.settings"), 0xFFFFFFFF, 1.2f);
    
    for (int i = 0; i < g_lang_count; i++) {
        uint32_t color = (i == g_selection) ? 0xFF00FFFF : 0xFFCCCCCC;
        char label[32];
        if (strcmp(g_lang_list[i], i18n_current_lang()) == 0) {
            snprintf(label, sizeof(label), "> %s", g_lang_list[i]);
            color = 0xFF00FF00;
        } else {
            snprintf(label, sizeof(label), "  %s", g_lang_list[i]);
        }
        font_draw_string(150, 80 + i * 25, label, color, 1.0f);
    }
    
    font_draw_string(20, 250, i18n_get("ctrl.back"), 0xFF888888, 0.8f);
    font_draw_string(400, 250, i18n_get("ctrl.select"), 0xFF888888, 0.8f);
}

Screen g_screen_settings = {
    settings_init,
    settings_update,
    settings_draw,
    NULL
};
