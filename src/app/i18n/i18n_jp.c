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

const char* g_lang_jp_entries[MSG_COUNT] = {
    [MSG_APP_TITLE]            = "ゲームダイアリー",
    [MSG_MENU_DASHBOARD]       = "ホーム",
    [MSG_MENU_STATS]           = "統計",
    [MSG_MENU_GAMES]           = "ゲーム",
    [MSG_MENU_SETTINGS]        = "設定",
    [MSG_MENU_ACTIVITY]        = "アクティビティ",
    [MSG_STATS_TOTAL_PLAYTIME] = "合計時間",
    [MSG_STATS_SESSIONS]       = "プレイ回数",
    [MSG_STATS_LAST_PLAYED]    = "最後にプレイ",
    [MSG_CTRL_BACK]            = "戻る",
    [MSG_CTRL_SELECT]          = "決定",
    [MSG_CTRL_MENU]            = "メニュー",
    [MSG_CTRL_CONFIG]          = "設定",
    [MSG_SETTINGS_LANGUAGE]    = "言語",
    [MSG_TOP_WEEK]             = "週",
    [MSG_TOP_MONTH]            = "月",
    [MSG_TOP_YEAR]             = "年",
    [MSG_TOP_ALL]              = "全期間",
    [MSG_DETAILS_FIRST_PLAYED] = "初プレイ",
    [MSG_CAT_PSP]              = "PSP",
    [MSG_CAT_PSX]              = "PSX",
    [MSG_CAT_MINIS]            = "ミニ",
    [MSG_CAT_HOMEBREW]         = "自作",
    [MSG_ERROR_NO_GAMES]       = "ゲームがありません",
    [MSG_CTRL_NAVIGATE]        = "移動",

    [MSG_DAY_SUN]              = "日",
    [MSG_DAY_MON]              = "月",
    [MSG_DAY_TUE]              = "火",
    [MSG_DAY_WED]              = "水",
    [MSG_DAY_THU]              = "木",
    [MSG_DAY_FRI]              = "金",
    [MSG_DAY_SAT]              = "土",

    // Format Time
    [MSG_STATS_NO_ACTIVITY]    = "最近の記録なし",
    [MSG_STATS_DAYS_ACTIVE]    = "活動日数",
    [MSG_STATS_NEVER]          = "なし",

    // 🇯🇵 DATE FORMAT (important change)
    [MSG_DATE_FORMAT]          = "%Y/%m/%d",
    [MSG_DATE_FORMAT_SHORT]    = "%m/%d",

    [MSG_DURATION_DAYS]        = "%d日",
    [MSG_DURATION_HOURS]       = "%d時間",
    [MSG_DURATION_MINS]        = "%d分",
    [MSG_DURATION_H_M]         = "%d時間 %d分",
    [MSG_DURATION_D_H_M]       = "%d日 %d時間 %d分",

    [MSG_STATS_MODE_WEEKLY]    = "週",
    [MSG_STATS_MODE_MONTHLY]   = "月",
    [MSG_STATS_MODE_YEARLY]    = "年",

    [MSG_MONTH_JAN]            = "1月",
    [MSG_MONTH_FEB]            = "2月",
    [MSG_MONTH_MAR]            = "3月",
    [MSG_MONTH_APR]            = "4月",
    [MSG_MONTH_MAY]            = "5月",
    [MSG_MONTH_JUN]            = "6月",
    [MSG_MONTH_JUL]            = "7月",
    [MSG_MONTH_AUG]            = "8月",
    [MSG_MONTH_SEP]            = "9月",
    [MSG_MONTH_OCT]            = "10月",
    [MSG_MONTH_NOV]            = "11月",
    [MSG_MONTH_DEC]            = "12月",

    [MSG_HINT_CHANGE_MODE]     = "← →で切替",
};
