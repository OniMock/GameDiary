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
 * @brief Loads plugin.dat; missing/invalid → hotkey off, icons on, capture all on.
 */
void plugin_dat_load(void);

/** @brief 1 = hotkey enabled, 0 = disabled. */
int plugin_dat_get_hotkey_enabled(void);

void plugin_dat_set_hotkey_enabled(int enabled);

/** @brief 1 = capture game icons (default when plugin.dat is missing). */
int plugin_dat_get_icon_enabled(void);

void plugin_dat_set_icon_enabled(int enabled);

/** @brief 1 = record PSP ISO/UMD games (default when the field is absent). */
int plugin_dat_get_capture_psp(void);

void plugin_dat_set_capture_psp(int enabled);

/** @brief 1 = record PS1 / POPS titles (default when the field is absent). */
int plugin_dat_get_capture_ps1(void);

void plugin_dat_set_capture_ps1(int enabled);

/** @brief 1 = record homebrew EBOOTs (default when the field is absent). */
int plugin_dat_get_capture_homebrew(void);

void plugin_dat_set_capture_homebrew(int enabled);

/** @brief 1 = record official PSP/APP content (default when the field is absent). */
int plugin_dat_get_capture_psp_app(void);

void plugin_dat_set_capture_psp_app(int enabled);

/** @brief 1 = record unrecognized / unknown titles (default when the field is absent). */
int plugin_dat_get_capture_unknown(void);

void plugin_dat_set_capture_unknown(int enabled);

/** @brief Writes plugin.dat synchronously. @return 0 on success. */
int plugin_dat_save(void);

#endif /* _APP_PLUGIN_DAT_H_ */
