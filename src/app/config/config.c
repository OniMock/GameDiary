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
        return config_save(); // Create with defaults
    }

    int res = sceIoRead(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    if (res != (int)sizeof(AppConfig)) {
        g_config.language = -1;
        return -1;
    }

    return 0;
}

int config_save(void) {
    if (g_config_path[0] == '\0') return -1;

    SceUID fd = sceIoOpen(g_config_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) return -1;

    int res = sceIoWrite(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    return (res == (int)sizeof(AppConfig)) ? 0 : -2;
}

AppConfig* config_get(void) {
    return &g_config;
}
