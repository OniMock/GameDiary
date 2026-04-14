#include <pspkernel.h>
#include <pspctrl.h>
#include "app/ui/screen.h"

static Screen* g_current_screen = NULL;
static u32 g_last_buttons = 0;

void screen_manager_set(Screen* screen) {
    if (g_current_screen && g_current_screen->destroy) {
        g_current_screen->destroy();
    }
    
    g_current_screen = screen;
    
    if (g_current_screen && g_current_screen->init) {
        g_current_screen->init();
    }
}

void screen_manager_update(void) {
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);
    
    u32 pressed = pad.Buttons & ~g_last_buttons;
    g_last_buttons = pad.Buttons;
    
    if (g_current_screen && g_current_screen->update) {
        g_current_screen->update(pad.Buttons, pressed);
    }
}

void screen_manager_draw(void) {
    if (g_current_screen && g_current_screen->draw) {
        g_current_screen->draw();
    }
}
