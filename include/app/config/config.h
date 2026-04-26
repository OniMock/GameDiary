/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_CONFIG_H
#define GAMEDIARY_CONFIG_H

/**
 * @file config.h
 * @brief Application configuration handling.
 */

typedef struct {
    int language; // -1 for AUTO, or index (LANG_EN, etc)
    int sfx_enabled; // 1 for ON, 0 for OFF
} AppConfig;

/**
 * @brief Initializes configuration with the application base path.
 * @param app_path Full path to the executable or its directory.
 */
void config_init(const char *app_path);

/**
 * @brief Loads the application configuration.
 * @return 0 on success, negative on error.
 */
int config_load(void);

/**
 * @brief Saves the current application configuration.
 * @return 0 on success, negative on error.
 */
int config_save(void);

/**
 * @brief Gets the current application configuration.
 * @return Pointer to the global configuration structure.
 */
AppConfig* config_get(void);

#endif // GAMEDIARY_CONFIG_H
