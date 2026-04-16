#ifndef GAMEDIARY_I18N_H
#define GAMEDIARY_I18N_H

#include <psptypes.h>
#include "app/render/image_resources.h"

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

/* Geometric symbols (UTF-8 in hexadecimal escapes) */
#define UI_SYM_CIRCLE_OPEN     "\xE2\x97\x8B"   /* ○ U+25CB */
#define UI_SYM_CIRCLE_FILLED   "\xE2\x97\x8F"   /* ● U+25CF */
#define UI_SYM_DIAMOND_OPEN    "\xE2\x97\x87"   /* ◇ U+25C7 */
#define UI_SYM_DIAMOND_FILLED  "\xE2\x97\x88"   /* ◆ U+25C8 (opcional) */
#define UI_SYM_SQUARE_OPEN     "\xE2\x96\xA1"   /* □ U+25A1 */
#define UI_SYM_SQUARE_FILLED   "\xE2\x96\xA0"   /* ■ U+25A0 */
#define UI_SYM_TRIANGLE_UP     "\xE2\x96\xB3"   /* △ U+25B3 */
#define UI_SYM_TRIANGLE_DOWN   "\xE2\x96\xBC"   /* ▼ U+25BC */
#define UI_SYM_TRIANGLE_LEFT   "\xE2\x97\x80"   /* ◀ U+25C0 */
#define UI_SYM_TRIANGLE_RIGHT  "\xE2\x96\xB6"   /* ▶ U+25B6 */
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

/**
 * Get the flag image resource for the current language.
 */
const ImageResource* i18n_get_current_flag(void);

/**
 * Get the flag image resource for a specific language index.
 */
const ImageResource* i18n_get_lang_flag(int index);

#endif // GAMEDIARY_I18N_H
