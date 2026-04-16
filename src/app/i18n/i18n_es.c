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
    [MSG_TOP_ALL]              = "Siempre",
    [MSG_CTRL_L] = "Ajustes",
    [MSG_CTRL_R] = "Estadísticas",

    [MSG_DAY_SUN] = "Dom",
    [MSG_DAY_MON] = "Lun",
    [MSG_DAY_TUE] = "Mar",
    [MSG_DAY_WED] = "Mié",
    [MSG_DAY_THU] = "Jue",
    [MSG_DAY_FRI] = "Vie",
    [MSG_DAY_SAT] = "Sáb",

    [MSG_STATS_NO_ACTIVITY] = "Sin actividad reciente",
    [MSG_STATS_DAYS_ACTIVE] = "D\xed\x61s Activos: %d", // Días
    [MSG_STATS_NEVER]       = "Nunca",
    [MSG_DATE_FORMAT]      = "%d/%m/%Y",
};
