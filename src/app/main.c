#include "common/common.h"
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

void setup_callbacks() {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

int main(void) {
    setup_callbacks();
    
    // 1. Core Rendering & UI
    renderer_init();
    font_init();
    
    // 2. Storage & Configuration
    storage_init("ms0:/PSP/COMMON/GameDiary");
    config_load();
    
    // 3. Systems Initialization
    i18n_init(config_get()->language);
    data_load_all();
    
    // 4. State Manager
    screen_manager_set(&g_screen_dashboard);
    
    // 5. Main Loop
    while (1) {
        renderer_start_frame();
        
        screen_manager_update();
        screen_manager_draw();
        
        renderer_end_frame();
    }
    
    // Cleanup
    font_cleanup();
    data_free();
    
    return 0;
}
