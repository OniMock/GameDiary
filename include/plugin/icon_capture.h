/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _PLUGIN_ICON_CAPTURE_H_
#define _PLUGIN_ICON_CAPTURE_H_

#include <psptypes.h>

/**
 * @brief Attempts to capture the game icon (ICON0.PNG) and save it to dest_path.
 * @param game_id The ID of the game (used for naming).
 * @param category The game category (PSP, PS1, etc).
 * @param dest_dir The directory where to save the icon.
 * @param executable_path Captured path to the EBOOT/ISO.
 */
void utils_capture_icon(const char *game_id, u8 category, const char *dest_dir, const char *executable_path);

#endif // _PLUGIN_ICON_CAPTURE_H_
