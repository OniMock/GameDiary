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
  * @file config.c
  * @brief Configuration system implementation.
  */
#include "app/config/config.h"
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include "common/db_schema.h"
#include "app/core/worker_thread.h"
#include "common/utils.h"

static AppConfig g_config;
static char g_config_path[256] = {0};

void config_init(const char *app_path) {
    /* If app_path is a file (e.g. EBOOT.PBP), extract the directory. */
    snprintf(g_config_path, sizeof(g_config_path), "%s", app_path);
    char *last_slash = strrchr(g_config_path, '/');
    if (last_slash) {
        *(last_slash + 1) = '\0'; // Keep the trailing slash
    }
    strcat(g_config_path, CONFIG_DAT);
}

int config_load(void) {
    if (g_config_path[0] == '\0') return -1;

    SceUID fd = sceIoOpen(g_config_path, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        // Default settings: Auto-detect language
        g_config.language = -1;
        g_config.sfx_enabled = 1;
        g_config.theme = 0;
        g_config.format_hours_only = 0;
        return config_write_file(&g_config); // Create with defaults
    }

    int res = sceIoRead(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    if (res > 0 && res < (int)sizeof(AppConfig)) {
        // It's an older version. Set new fields to default.
        if (res <= 8) { // 8 is the size of the old struct (language + sfx_enabled)
            g_config.theme = 0; // Dark theme
            g_config.format_hours_only = 0;
        } else if (res <= 12) { // 12 is the size of the old struct (+ theme)
            g_config.format_hours_only = 0;
        }
        config_write_file(&g_config); // Save the updated struct size
        return 0;
    }

    if (res != (int)sizeof(AppConfig)) {
        g_config.language = -1;
        g_config.sfx_enabled = 1;
        g_config.theme = 0;
        g_config.format_hours_only = 0;
        return -1;
    }

    return 0;
}

int config_write_file(const AppConfig* cfg) {
    if (g_config_path[0] == '\0') return -1;

    SceUID fd = sceIoOpen(g_config_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) return -1;

    int res = sceIoWrite(fd, cfg, sizeof(AppConfig));
    sceIoClose(fd);

    return (res == (int)sizeof(AppConfig)) ? 0 : -2;
}

int config_save(void) {
    int res = worker_enqueue_save_config(&g_config);
    if (res < 0) {
        /* Fallback to synchronous save if worker queue is full or not running */
        return config_write_file(&g_config);
    }
    return 0;
}

AppConfig* config_get(void) {
    return &g_config;
}
