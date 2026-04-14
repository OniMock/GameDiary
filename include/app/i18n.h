#ifndef GAMEDIARY_I18N_H
#define GAMEDIARY_I18N_H

#include <psptypes.h>

typedef struct {
    u32 key_hash;
    const char* value;
} TranslationEntry;

typedef struct {
    const char* lang_code;
    const char* lang_name;
    const TranslationEntry* entries;
    int count;
} LanguagePack;

extern LanguagePack g_lang_en;
extern LanguagePack g_lang_pt;
extern LanguagePack g_lang_es;

/**
 * Initialize the i18n system with a language code.
 */
void i18n_init(const char* lang_code);

/**
 * Switch the active language at runtime.
 */
void i18n_set_language(const char* lang_code);

/**
 * Get a translated string for a key.
 * If not found, returns the key itself.
 */
const char* i18n_get(const char* key);

/**
 * Get the current language code.
 */
const char* i18n_current_lang(void);

/**
 * Get the total number of supported languages.
 */
int i18n_get_lang_count(void);

/**
 * Get a language pack by index for selection menus.
 */
const LanguagePack* i18n_get_lang_pack(int index);

#endif // GAMEDIARY_I18N_H
