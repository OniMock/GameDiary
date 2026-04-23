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

// Simplified Chinese
const char* g_lang_cn_entries[MSG_COUNT] = {
    [MSG_MENU_STATS]           = "统计",
    [MSG_MENU_GAMES]           = "游戏",
    [MSG_MENU_SETTINGS]        = "设置",
    [MSG_MENU_ACTIVITY]        = "活动",
    [MSG_STATS_TOTAL_PLAYTIME] = "总时间",
    [MSG_STATS_SESSIONS]       = "游玩次数",
    [MSG_STATS_LAST_PLAYED]    = "最近游玩",
    [MSG_CTRL_BACK]            = "返回",
    [MSG_CTRL_SELECT]          = "确认",
    [MSG_CTRL_MENU]            = "菜单",
    [MSG_CTRL_CONFIG]          = "设置",
    [MSG_SETTINGS_LANGUAGE]    = "语言",
    [MSG_TOP_WEEK]             = "周",
    [MSG_TOP_MONTH]            = "月",
    [MSG_TOP_YEAR]             = "年",
    [MSG_TOP_ALL]              = "全部",
    [MSG_DETAILS_FIRST_PLAYED] = "首次游玩",
    [MSG_CAT_PSP]              = "PSP",
    [MSG_CAT_PSX]              = "PSX",
    [MSG_CAT_MINIS]            = "迷你",
    [MSG_CAT_HOMEBREW]         = "自制",
    [MSG_ERROR_NO_GAMES]       = "没有游戏",
    [MSG_CTRL_NAVIGATE]        = "移动",

    [MSG_DAY_SUN]              = "日",
    [MSG_DAY_MON]              = "一",
    [MSG_DAY_TUE]              = "二",
    [MSG_DAY_WED]              = "三",
    [MSG_DAY_THU]              = "四",
    [MSG_DAY_FRI]              = "五",
    [MSG_DAY_SAT]              = "六",

    // Format Time
    [MSG_STATS_NO_ACTIVITY]    = "无最近记录",
    [MSG_STATS_DAYS_ACTIVE]    = "活跃天数",
    [MSG_STATS_NEVER]          = "从未",

    // 🇨🇳 DATE FORMAT (important)
    [MSG_DATE_FORMAT]          = "%Y年%m月%d日",
    [MSG_DATE_FORMAT_SHORT]    = "%m月%d日",

    [MSG_DURATION_DAYS]        = "%d天",
    [MSG_DURATION_HOURS]       = "%d小时",
    [MSG_DURATION_MINS]        = "%d分钟",
    [MSG_DURATION_H_M]         = "%d小时%d分钟",
    [MSG_DURATION_D_H_M]       = "%d天%d小时%d分钟",

    [MSG_STATS_MODE_WEEKLY]    = "周",
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

    [MSG_FILTER]               = "筛选",

    [MSG_HINT_HELPER]              = "[ L ]: 帮助",
    [MSG_HELP_TITLE]               = "帮助",
    [MSG_HELP_BTN_X_SELECT]        = "[ X ]: 选择",
    [MSG_HELP_BTN_X_CONFIRM]       = "[ X ]: 确认",
    [MSG_HELP_BTN_X_CHANGE]        = "[ X ]: 更改",
    [MSG_HELP_BTN_O_BACK]          = "[ O ]: 返回",
    [MSG_HELP_BTN_START_MENU]      = "[ START ]: 菜单",
    [MSG_HELP_BTN_SELECT_CONFIG]   = "[ SELECT ]: 设置",
    [MSG_HELP_BTN_ARROWS_NAVIGATE] = "[ ↑ ↓ ]: 导航",
    [MSG_HELP_BTN_ANALOG_NAVIGATE] = "[ ← → ] 或 [ ◉ ]: 导航",
    [MSG_HELP_BTN_ANALOG_FILTER]   = "[ ← → ] 或 [ ◉ ]: 更改过滤",
    [MSG_HELP_BTN_SQUARE_FILTER]   = "[ □ ]: 过滤",
    [MSG_HELP_BTN_TRIANGLE_STATS]  = "[ △ ]: 统计",
};
