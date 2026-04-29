/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

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
    LANG_JP,
    LANG_CN,
    LANG_DE,

    LANG_COUNT,
    LANG_AUTO = -1
} LanguageId;

typedef enum {
    MSG_MENU_STATS = 0,
    MSG_MENU_GAMES,
    MSG_MENU_SETTINGS,
    MSG_MENU_ACTIVITY,
    MSG_STATS_TOTAL_PLAYTIME,
    MSG_STATS_SESSIONS,
    MSG_STATS_LAST_PLAYED,
    MSG_CTRL_BACK,
    MSG_CTRL_SELECT,
    MSG_CTRL_MENU,
    MSG_CTRL_CONFIG,
    MSG_SETTINGS_LANGUAGE,
    MSG_SETTINGS_ABOUT,
    MSG_SETTINGS_SUPPORT,
    MSG_SETTINGS_SFX,
    MSG_SFX_ON,
    MSG_SFX_OFF,
    MSG_ABOUT_TITLE,
    MSG_ABOUT_DESC,
    MSG_ABOUT_GITHUB,
    MSG_ABOUT_VERSION,
    MSG_ABOUT_PSP_SDK,
    MSG_ABOUT_DEVELOPER,
    MSG_SUPPORT_DESC,
    MSG_SUPPORT_COFFEE,
    MSG_SUPPORT_WALLET,
    MSG_TOP_WEEK,
    MSG_TOP_MONTH,
    MSG_TOP_YEAR,
    MSG_TOP_ALL,
    MSG_DETAILS_FIRST_PLAYED,
    MSG_CAT_PSP,
    MSG_CAT_PSX,
    MSG_CAT_MINIS,
    MSG_CAT_HOMEBREW,
    MSG_ERROR_NO_GAMES,
    MSG_CTRL_NAVIGATE,

    MSG_DAY_SUN,
    MSG_DAY_MON,
    MSG_DAY_TUE,
    MSG_DAY_WED,
    MSG_DAY_THU,
    MSG_DAY_FRI,
    MSG_DAY_SAT,

    MSG_STATS_NO_ACTIVITY,
    MSG_STATS_DAYS_ACTIVE,
    MSG_STATS_NEVER,
    MSG_DATE_FORMAT,
    MSG_DATE_FORMAT_SHORT,
    MSG_DURATION_DAYS,
    MSG_DURATION_HOURS,
    MSG_DURATION_MINS,
    MSG_DURATION_H_M,
    MSG_DURATION_D_H_M,

    MSG_STATS_MODE_WEEKLY,
    MSG_STATS_MODE_MONTHLY,
    MSG_STATS_MODE_MONTHS,
    MSG_STATS_MODE_YEARLY,

    MSG_MONTH_JAN,
    MSG_MONTH_FEB,
    MSG_MONTH_MAR,
    MSG_MONTH_APR,
    MSG_MONTH_MAY,
    MSG_MONTH_JUN,
    MSG_MONTH_JUL,
    MSG_MONTH_AUG,
    MSG_MONTH_SEP,
    MSG_MONTH_OCT,
    MSG_MONTH_NOV,
    MSG_MONTH_DEC,

    MSG_MONTH_SHORT_JAN,
    MSG_MONTH_SHORT_FEB,
    MSG_MONTH_SHORT_MAR,
    MSG_MONTH_SHORT_APR,
    MSG_MONTH_SHORT_MAY,
    MSG_MONTH_SHORT_JUN,
    MSG_MONTH_SHORT_JUL,
    MSG_MONTH_SHORT_AUG,
    MSG_MONTH_SHORT_SEP,
    MSG_MONTH_SHORT_OCT,
    MSG_MONTH_SHORT_NOV,
    MSG_MONTH_SHORT_DEC,

    MSG_FILTER,

    MSG_HINT_HELPER,
    MSG_HELP_TITLE,
    MSG_HELP_BTN_X_SELECT,
    MSG_HELP_BTN_X_CONFIRM,
    MSG_HELP_BTN_X_CHANGE,
    MSG_HELP_BTN_O_BACK,
    MSG_HELP_BTN_START_MENU,
    MSG_HELP_BTN_ARROWS_NAVIGATE,
    MSG_HELP_BTN_ANALOG_NAVIGATE,
    MSG_HELP_BTN_ANALOG_FILTER,
    MSG_HELP_BTN_SQUARE_FILTER,
    MSG_HELP_BTN_TRIANGLE_STATS,
    MSG_HELP_BTN_SELECT_CONFIG,
    MSG_ABOUT_DATE,
    MSG_HELP_CONTROLS,
    MSG_HELP_INFO_LABEL,

    /* Section descriptions */
    MSG_HELP_DESC_GAMES,
    MSG_HELP_DESC_STATS,
    MSG_HELP_DESC_ACTIVITY,
    MSG_HELP_DESC_SETTINGS,
    MSG_HELP_DESC_DETAILS,
    MSG_HELP_DESC_MAIN_MENU,
    MSG_HELP_DESC_LANG_SELECT,
    MSG_HELP_DESC_ABOUT,
    MSG_HELP_DESC_SUPPORT,
    MSG_HELP_CLOSE_HINT,

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

/**
 * Get the flag image resource for the current language.
 */
const ImageResource* i18n_get_current_flag(void);

/**
 * Get the flag image resource for a specific language index.
 */
const ImageResource* i18n_get_lang_flag(int index);

/**
 * Get the LanguageId enum value for the Nth language in alphabetical
 * (A-Z) display order. Use this for sorted UI lists.
 *
 * @param sorted_pos  Visual position in the A-Z list (0 = first).
 * @return            The corresponding LanguageId, or LANG_EN on error.
 */
LanguageId i18n_get_sorted_lang_index(int sorted_pos);

#endif // GAMEDIARY_I18N_H
