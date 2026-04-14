#ifndef GAMEDIARY_SCREEN_H
#define GAMEDIARY_SCREEN_H

#include <psptypes.h>

typedef struct {
    void (*init)(void);
    void (*update)(u32 buttons, u32 pressed);
    void (*draw)(void);
    void (*destroy)(void);
} Screen;

void screen_manager_set(Screen* screen);
void screen_manager_update(void);
void screen_manager_draw(void);

// Screen specific helpers
void game_details_set_idx(int idx);

// Screens
extern Screen g_screen_dashboard;
extern Screen g_screen_stats;
extern Screen g_screen_game_list;
extern Screen g_screen_game_details;
extern Screen g_screen_settings;

#endif // GAMEDIARY_SCREEN_H
