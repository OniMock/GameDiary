#include "app/i18n.h"

// Using ISO-8859-1 escape sequences to ensure compatibility with ltn8.pgf
// ê = \xea, ç = \xe7, á = \xe1, ã = \xe3, õ = \xf5, ó = \xf3, í = \xed
const char* g_lang_pt_entries[MSG_COUNT] = {
    [MSG_APP_TITLE]            = "Di\xe1rio de Jogo",
    [MSG_MENU_DASHBOARD]       = "In\xedcio",
    [MSG_MENU_STATS]           = "Estat\xedsticas",
    [MSG_MENU_GAMES]           = "Jogos",
    [MSG_MENU_SETTINGS]        = "Ajustes",
    [MSG_STATS_TOTAL_PLAYTIME] = "Tempo Total",
    [MSG_STATS_SESSIONS]       = "Sess\xf5es",
    [MSG_STATS_LAST_PLAYED]    = "\xdaltimo Jogo",
    [MSG_MENU_GAMES_PRESS_X]   = "Pressione X para os jogos",
    [MSG_CTRL_BACK]            = "Voltar",
    [MSG_CTRL_SELECT]          = "Selecionar",
    [MSG_SETTINGS_LANGUAGE]    = "Idioma",
    [MSG_TOP_WEEK]             = "Semana",
    [MSG_TOP_MONTH]            = "M\xeas",
    [MSG_TOP_ALL]              = "Sempre"
};
