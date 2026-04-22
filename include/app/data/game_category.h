/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_GAME_CATEGORY_H
#define GAMEDIARY_GAME_CATEGORY_H

#include <psptypes.h>
#include "common/models.h"

/**
 * @file game_category.h
 * @brief Centralized game category management and display.
 */

#define FILTER_ALL 255

/**
 * @brief Get the localized name for a category.
 * @param category One of CAT_* constants.
 * @return Localized string.
 */
const char* game_category_get_name(u8 category);

/**
 * @brief Normalizes a category for filtering and display.
 * (e.g. treats CAT_VSH as CAT_PSP).
 */
u8 game_category_normalize(u8 category);

/**
 * @brief Scans the current game data to find which categories have at least one game.
 * @param out_categories Buffer of at least CAT_UNKNOWN size.
 * @return Number of unique categories found.
 */
int game_category_get_available(u8 *out_categories);

#endif // GAMEDIARY_GAME_CATEGORY_H
