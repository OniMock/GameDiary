/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_SCREEN_H
#define GAMEDIARY_SCREEN_H

#include <psptypes.h>

/**
 * @brief Screen interface for the application.
 *
 * This struct defines the interface for a screen in the application.
 * Each screen has an init, update, draw, and destroy function.
 */
typedef struct {
    void (*init)(void);
    void (*update)(u32 buttons, u32 pressed);
    void (*draw)(void);
    void (*destroy)(void);
} Screen;

void screen_manager_set(Screen* screen);
void screen_manager_push(Screen* screen);
void screen_manager_pop(void);
void screen_manager_update(void);
void screen_manager_draw(void);

// Screen specific helpers
void game_details_set_idx(int idx);

// Screens
extern Screen g_screen_main_menu;
extern Screen g_screen_stats;
extern Screen g_screen_activity;
extern Screen g_screen_game_list;
extern Screen g_screen_game_details;
extern Screen g_screen_settings;
extern Screen g_screen_about;
extern Screen g_screen_support;
extern Screen g_screen_language_select;

#endif // GAMEDIARY_SCREEN_H
