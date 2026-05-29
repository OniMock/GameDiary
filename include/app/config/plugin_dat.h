/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file plugin_dat.h
 * @brief App-side read/write of PSP/COMMON/GameDiary/plugin.dat
 */

#ifndef _APP_PLUGIN_DAT_H_
#define _APP_PLUGIN_DAT_H_

/**
 * @brief Loads plugin.dat; missing/invalid → hotkey disabled (0).
 */
void plugin_dat_load(void);

/** @brief 1 = hotkey enabled, 0 = disabled. */
int plugin_dat_get_hotkey_enabled(void);

void plugin_dat_set_hotkey_enabled(int enabled);

/** @brief Writes plugin.dat synchronously. @return 0 on success. */
int plugin_dat_save(void);

#endif /* _APP_PLUGIN_DAT_H_ */
