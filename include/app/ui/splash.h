/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_SPLASH_H
#define GAMEDIARY_SPLASH_H

#include "app/ui/screen.h"
#include <stdbool.h>

extern Screen g_screen_splash;

bool splash_is_loading(void);
void splash_do_load_tasks(void);

#endif // GAMEDIARY_SPLASH_H
