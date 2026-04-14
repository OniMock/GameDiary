#include "app/config/config.h"
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>

#define CONFIG_PATH "ms0:/PSP/COMMON/GameDiary/config.dat"

static AppConfig g_config;

int config_load(void) {
    SceUID fd = sceIoOpen(CONFIG_PATH, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        // Default settings: Auto-detect language
        g_config.language = -1; 
        return config_save(); // Create with defaults
    }

    int res = sceIoRead(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    if (res != sizeof(AppConfig)) {
        g_config.language = -1;
        return -1;
    }

    return 0;
}

int config_save(void) {
    SceUID fd = sceIoOpen(CONFIG_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) return -1;

    int res = sceIoWrite(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    return (res == sizeof(AppConfig)) ? 0 : -2;
}

AppConfig* config_get(void) {
    return &g_config;
}
