/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_DEBUG_H
#define GAMEDIARY_DEBUG_H

#include <psptypes.h>

/**
 * @file debug.h
 * @brief Thread-safe debug logging system for GameDiary.
 */

/**
 * @brief Initialize debug system (creates directory if needed).
 */
void debug_init(void);

/**
 * @brief Detailed log with system context.
 * Only active if GDIARY_DEBUG is defined.
 *
 * @param module Module name (e.g., "NET", "UI")
 * @param fmt Format string
 */
void debug_log(const char* module, const char* fmt, ...);

#endif // GAMEDIARY_DEBUG_H
