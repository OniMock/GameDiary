#include "app/i18n.h"

// Using ISO-8859-1 escape sequences to ensure compatibility with ltn8.pgf
// í = \xed, ñ = \xf1, á = \xe1, ó = \xf3, ú = \xfa
const char* g_lang_es_entries[MSG_COUNT] = {
    [MSG_APP_TITLE]            = "Diario de Juego",
    [MSG_MENU_DASHBOARD]       = "Inicio",
    [MSG_MENU_STATS]           = "Estadísticas",
    [MSG_MENU_GAMES]           = "Juegos",
    [MSG_MENU_SETTINGS]        = "Ajustes",
    [MSG_STATS_TOTAL_PLAYTIME] = "Tiempo Total",
    [MSG_STATS_SESSIONS]       = "Sesiones",
    [MSG_STATS_LAST_PLAYED]    = "Último Juego",
    [MSG_MENU_GAMES_PRESS_X]   = "Presione X para ver juegos",
    [MSG_CTRL_BACK]            = "Voltar",
    [MSG_CTRL_SELECT]          = "Seleccionar",
    [MSG_SETTINGS_LANGUAGE]    = "Idioma",
    [MSG_TOP_WEEK]             = "Semana",
    [MSG_TOP_MONTH]            = "Mes",
    [MSG_TOP_ALL]              = "Siempre"
};
