#include "app/i18n.h"
#include <string.h>
#include <stdlib.h>

/* --- Static Repository of Keys --- */

static const char* i18n_keys[] = {
    "app.title",
    "menu.dashboard",
    "menu.stats",
    "menu.games",
    "menu.settings",
    "stats.total_playtime",
    "stats.sessions",
    "stats.last_played",
    "menu.games_press_x",
    "ctrl.back",
    "ctrl.select",
    "settings.language",
    "top.week",
    "top.month",
    "top.all"
};

#define ENTRY_COUNT (sizeof(i18n_keys) / sizeof(i18n_keys[0]))

static const LanguagePack* g_languages[] = {
    &g_lang_en,
    &g_lang_pt,
    &g_lang_es
};

static const LanguagePack* g_current_pack = NULL;
static const int g_lang_count = 3;

/* --- Logic --- */

static u32 i18n_hash(const char *str) {
    u32 hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

// Internal helper to populate hashes in the packs once
static void i18n_initialize_packs(void) {
    static int initialized = 0;
    if (initialized) return;

    for (int l = 0; l < g_lang_count; l++) {
        LanguagePack* lp = (LanguagePack*)g_languages[l];
        for (int i = 0; i < lp->count; i++) {
            ((TranslationEntry*)lp->entries)[i].key_hash = i18n_hash(i18n_keys[i]);
        }
    }
    initialized = 1;
}

void i18n_init(const char* lang_code) {
    i18n_initialize_packs();
    i18n_set_language(lang_code);
}

void i18n_set_language(const char* lang_code) {
    for (int i = 0; i < g_lang_count; i++) {
        if (strcmp(g_languages[i]->lang_code, lang_code) == 0) {
            g_current_pack = g_languages[i];
            return;
        }
    }
    // Fallback if not found
    g_current_pack = g_languages[0];
}

const char* i18n_get(const char* key) {
    if (!g_current_pack) return key;

    u32 h = i18n_hash(key);
    for (int i = 0; i < g_current_pack->count; i++) {
        if (g_current_pack->entries[i].key_hash == h) {
            return g_current_pack->entries[i].value;
        }
    }
    return key;
}

const char* i18n_current_lang(void) {
    return g_current_pack ? g_current_pack->lang_code : "en";
}

int i18n_get_lang_count(void) {
    return g_lang_count;
}

const LanguagePack* i18n_get_lang_pack(int index) {
    if (index >= 0 && index < g_lang_count) {
        return g_languages[index];
    }
    return NULL;
}
