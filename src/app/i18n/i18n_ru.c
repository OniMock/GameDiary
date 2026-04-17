/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/i18n/i18n.h"

const char* g_lang_ru_entries[MSG_COUNT] = {
    [MSG_APP_TITLE]            = "Игровой дневник",
    [MSG_MENU_DASHBOARD]       = "Панель",
    [MSG_MENU_STATS]           = "Статистика",
    [MSG_MENU_GAMES]           = "Игры",
    [MSG_MENU_SETTINGS]        = "Настройки",
    [MSG_STATS_TOTAL_PLAYTIME] = "Общее время игры",
    [MSG_STATS_SESSIONS]       = "Сеансы",
    [MSG_STATS_LAST_PLAYED]    = "Последняя игра",
    [MSG_MENU_GAMES_PRESS_X]   = "Нажмите X, чтобы открыть игры",
    [MSG_CTRL_BACK]            = "Назад",
    [MSG_CTRL_SELECT]          = "Выбрать",
    [MSG_SETTINGS_LANGUAGE]    = "Язык",
    [MSG_TOP_WEEK]             = "Неделя",
    [MSG_TOP_MONTH]            = "Месяц",
    [MSG_TOP_ALL]              = "Все время",
    [MSG_ERROR_NO_GAMES]       = "Игры не найдены",
    [MSG_CTRL_L] = "Настройки",
    [MSG_CTRL_R] = "Статистика",

    [MSG_DAY_SUN] = "Вс",
    [MSG_DAY_MON] = "Пн",
    [MSG_DAY_TUE] = "Вт",
    [MSG_DAY_WED] = "Ср",
    [MSG_DAY_THU] = "Чт",
    [MSG_DAY_FRI] = "Пт",
    [MSG_DAY_SAT] = "Сб",

    // Format Time
    [MSG_STATS_NO_ACTIVITY] = "Нет активности",
    [MSG_STATS_DAYS_ACTIVE] = "Дней активности: %d",
    [MSG_STATS_NEVER]       = "Никогда",
    [MSG_DATE_FORMAT]      = "%d.%m.%Y",
    [MSG_DATE_FORMAT_SHORT] = "%d.%m",
    [MSG_DURATION_DAYS]    = "%dд",
    [MSG_DURATION_HOURS]   = "%uч",
    [MSG_DURATION_MINS]    = "%uм",
    [MSG_DURATION_H_M]     = "%uч %uм",
    [MSG_DURATION_D_H_M]   = "%dд %uч %uм",
};
