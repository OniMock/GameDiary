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
    [MSG_SETTINGS_ABOUT]       = "关于",
    [MSG_SETTINGS_SUPPORT]     = "支持",
    [MSG_SETTINGS_SFX]         = "声音 (SFX)",
    [MSG_SFX_ON]               = "开启",
    [MSG_SFX_OFF]              = "关闭",
    [MSG_TOP_WEEK]             = "周",
    [MSG_TOP_MONTH]            = "月",
    [MSG_TOP_YEAR]             = "年",
    [MSG_TOP_ALL]              = "所有",
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
    [MSG_DATE_FORMAT]          = "%Y/%m/%d",
    [MSG_DATE_FORMAT_SHORT]    = "%m/%d",

    [MSG_DURATION_DAYS]        = "%d日",
    [MSG_DURATION_HOURS]       = "%d時",
    [MSG_DURATION_MINS]        = "%d分",
    [MSG_DURATION_H_M]         = "%d時%d分",
    [MSG_DURATION_D_H_M]       = "%d日%d時%d分",

    [MSG_STATS_MODE_WEEKLY]    = "周",
    [MSG_STATS_MODE_MONTHLY]   = "月",
    [MSG_STATS_MODE_MONTHS]    = "按月",
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
    [MSG_HELP_BTN_SQUARE_FILTER]   = "[ ■ ]: 过滤",
    [MSG_HELP_BTN_TRIANGLE_STATS]  = "[ △ ]: 统计",
    [MSG_HELP_CONTROLS]            = "操作:",
    [MSG_HELP_INFO_LABEL]          = "信息:",
    [MSG_HELP_DESC_GAMES]          = "可按类别筛选，查看总时间和最近游玩记录。",
    [MSG_HELP_DESC_STATS]          = "通过图表查看你的游戏时间。在周、月、按月和年之间切换，看看哪些游戏占用了你最多的时间。",
    [MSG_HELP_DESC_ACTIVITY]       = "按时间顺序显示最近的游玩记录。",
    [MSG_HELP_DESC_SETTINGS]       = "调整语言和其他设置。",
    [MSG_HELP_DESC_DETAILS]        = "查看游戏详细信息和完整记录。",
    [MSG_HELP_DESC_MAIN_MENU]      = "主菜单，可进入游戏、统计、记录或设置。",
    [MSG_HELP_DESC_LANG_SELECT]    = "选择界面语言，立即生效。",

    [MSG_HELP_DESC_ABOUT]          = "有关应用程序、版本和开发者信息的说明。",
    [MSG_HELP_CLOSE_HINT]          = "[ X / O ]: 关闭",

    [MSG_ABOUT_TITLE]              = "关于 %s",
    [MSG_ABOUT_DESC]               = "%s 是一个用于 PSP 的游戏时间追踪系统，旨在帮助您管理您的游戏库和收藏。",
    [MSG_ABOUT_GITHUB]             = "github.com/OniMock/%s",
    [MSG_ABOUT_VERSION]            = "版本",
    [MSG_ABOUT_PSP_SDK]            = "PSP SDK",
    [MSG_ABOUT_DATE]               = "日期",
    [MSG_ABOUT_DEVELOPER]          = "开发者：OniMock",
    [MSG_HELP_DESC_SUPPORT]        = "如何支持开发者并帮助项目。",
    [MSG_SUPPORT_DESC]             = "如果你喜欢我的作品并希望支持项目的持续开发，可以考虑进行捐赠。这不是必须的，但对项目发展非常有帮助！",
    [MSG_SUPPORT_COFFEE]           = "Buy Me A Coffee",
    [MSG_SUPPORT_WALLET]           = "Wallet EVM",
};
