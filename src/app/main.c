#include "common/common.h"
#include "common/utils.h"
#include "common/storage.h"
#include "app/i18n.h"
#include "app/config/config.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/ui/screen.h"
#include "app/data/data_loader.h"
#include <pspkernel.h>
#include <pspctrl.h>

PSP_MODULE_INFO("GameDiaryApp", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

int exit_callback(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

int main(int argc, char *argv[]) {
    setup_callbacks();

    /* 1. Core Rendering & UI
     * Font data is embedded in the binary via font_*_embed.c — no file I/O needed.
     * Works identically on real PSP and PPSSPP without copying any extra files. */
    renderer_init();
    font_init();

    /* 2. Storage & Configuration
     * Use argv[0] to determine application root for local config.dat. */
    if (argc > 0 && argv[0]) {
        config_init(argv[0]);
    } else {
        config_init("ms0:/PSP/GAME/GameDiary/EBOOT.PBP"); // Fallback
    }
    
    config_load();

    /* Initialize storage with dynamic device prefix (ms0: vs ef0:) */
    char base_path[128];
    snprintf(base_path, sizeof(base_path), "%s/PSP/COMMON/GameDiary", utils_get_device_prefix());
    storage_init(base_path);

    /* 3. Systems Initialization */
    i18n_init(config_get()->language);
    data_load_all();

    /* 4. State Manager */
    screen_manager_set(&g_screen_dashboard);

    /* 5. Main Loop */
    while (1) {
        renderer_start_frame();
        screen_manager_update();
        screen_manager_draw();
        renderer_end_frame();
    }

    /* Cleanup */
    font_cleanup();
    data_free();

    return 0;
}
