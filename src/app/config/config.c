#include "app/config/config.h"
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>

#include "common/utils.h"

static AppConfig g_config;

int config_load(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/PSP/COMMON/GameDiary/config.dat", utils_get_device_prefix());

    SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
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
    char path[256];
    snprintf(path, sizeof(path), "%s/PSP/COMMON/GameDiary/config.dat", utils_get_device_prefix());

    SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) return -1;

    int res = sceIoWrite(fd, &g_config, sizeof(AppConfig));
    sceIoClose(fd);

    return (res == sizeof(AppConfig)) ? 0 : -2;
}

AppConfig* config_get(void) {
    return &g_config;
}
