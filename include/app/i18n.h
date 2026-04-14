#ifndef GAMEDIARY_I18N_H
#define GAMEDIARY_I18N_H

/**
 * @file i18n.h
 * @brief Runtime internationalization system for GameDiary.
 */

/**
 * @brief Initializes the i18n system and loads the default language.
 * @param lang_code The language code to load (e.g., "en", "pt").
 * @return 0 on success, negative on error.
 */
int i18n_init(const char *lang_code);

/**
 * @brief Loads a specific language file from the strings directory.
 * @param lang_code The language code to load.
 * @return 0 on success, negative on error.
 */
int i18n_load(const char *lang_code);

/**
 * @brief Retrieves a translated string for a given key.
 * @param key The translation key.
 * @return The translated string, or the key itself if not found.
 */
const char* i18n_get(const char *key);

/**
 * @brief Cleans up i18n resources.
 */
void i18n_cleanup(void);

/**
 * @brief Gets the current loaded language code.
 * @return String representing the current language code.
 */
const char* i18n_current_lang(void);

#endif // GAMEDIARY_I18N_H
