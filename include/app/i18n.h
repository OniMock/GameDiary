#ifndef GAMEDIARY_I18N_H
#define GAMEDIARY_I18N_H

#include <psptypes.h>

/**
 * @file i18n.h
 * @brief Internationalization system using indexed enums for performance.
 */

typedef enum {
    LANG_EN = 0,
    LANG_PT,
    LANG_ES,
    LANG_RU,

    LANG_COUNT,
    LANG_AUTO = -1
} LanguageId;

typedef enum {
    MSG_APP_TITLE = 0,
    MSG_MENU_DASHBOARD,
    MSG_MENU_STATS,
    MSG_MENU_GAMES,
    MSG_MENU_SETTINGS,
    MSG_STATS_TOTAL_PLAYTIME,
    MSG_STATS_SESSIONS,
    MSG_STATS_LAST_PLAYED,
    MSG_MENU_GAMES_PRESS_X,
    MSG_CTRL_BACK,
    MSG_CTRL_SELECT,
    MSG_SETTINGS_LANGUAGE,
    MSG_TOP_WEEK,
    MSG_TOP_MONTH,
    MSG_TOP_ALL,
    MSG_ERROR_NO_GAMES,
    MSG_CTRL_L,
    MSG_CTRL_R,

    MSG_DAY_SUN,
    MSG_DAY_MON,
    MSG_DAY_TUE,
    MSG_DAY_WED,
    MSG_DAY_THU,
    MSG_DAY_FRI,
    MSG_DAY_SAT,

    MSG_STATS_NO_ACTIVITY,

    MSG_COUNT
} MessageId;

/**
 * Global message pointer for direct access.
 * Usage: g_i18n_msg[MSG_APP_TITLE]
 */
extern const char **g_i18n_msg;

/**
 * Initialize the i18n system.
 * @param force_lang Language index to use, or LANG_AUTO to use system setting.
 */
void i18n_init(int force_lang);

/**
 * Change language at runtime.
 */
void i18n_set_language(int lang_index);

/**
 * Get translated string with optional safety checks.
 */
const char* i18n_get(MessageId id);

/**
 * Get the current language index.
 */
int i18n_current_lang(void);

/**
 * Get the language name by index (e.g., "English").
 */
const char* i18n_get_lang_name(int index);

#endif // GAMEDIARY_I18N_H
