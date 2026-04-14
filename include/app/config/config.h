#ifndef GAMEDIARY_CONFIG_H
#define GAMEDIARY_CONFIG_H

/**
 * @file config.h
 * @brief Application configuration handling.
 */

typedef struct {
    char language[8];
    // Add other settings here as needed (e.g., theme, auto-save)
} AppConfig;

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
