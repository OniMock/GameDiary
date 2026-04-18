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

const char* g_lang_pt_entries[MSG_COUNT] = {
    [MSG_APP_TITLE]            = "Diário de Jogo",
    [MSG_MENU_DASHBOARD]       = "Início",
    [MSG_MENU_STATS]           = "Estatísticas",
    [MSG_MENU_GAMES]           = "Jogos",
    [MSG_MENU_SETTINGS]        = "Menu",
    [MSG_STATS_TOTAL_PLAYTIME] = "Tempo Total",
    [MSG_STATS_SESSIONS]       = "Sessões",
    [MSG_STATS_LAST_PLAYED]    = "Último Jogo",
    [MSG_CTRL_BACK]            = "Voltar",
    [MSG_CTRL_SELECT]          = "Selecionar",
    [MSG_CTRL_MENU]            = "Menu",
    [MSG_CTRL_CONFIG]          = "Config",
    [MSG_SETTINGS_LANGUAGE]    = "Idioma",
    [MSG_TOP_WEEK]             = "Semana",
    [MSG_TOP_MONTH]            = "Mês",
    [MSG_TOP_ALL]              = "Sempre",
    [MSG_ERROR_NO_GAMES]       = "Nenhum jogo encontrado",
    [MSG_CTRL_NAVIGATE]        = "Navegar",

    [MSG_DAY_SUN]              = "Dom",
    [MSG_DAY_MON]              = "Seg",
    [MSG_DAY_TUE]              = "Ter",
    [MSG_DAY_WED]              = "Qua",
    [MSG_DAY_THU]              = "Qui",
    [MSG_DAY_FRI]              = "Sex",
    [MSG_DAY_SAT]              = "Sáb",

    // Format Time
    [MSG_STATS_NO_ACTIVITY]    = "Sem atividade recente",
    [MSG_STATS_DAYS_ACTIVE]    = "Dias Ativos: %d",
    [MSG_STATS_NEVER]          = "Nunca",
    [MSG_DATE_FORMAT]          = "%d/%m/%Y",
    [MSG_DATE_FORMAT_SHORT]    = "%d/%m",
    [MSG_DURATION_DAYS]        = "%dd",
    [MSG_DURATION_HOURS]       = "%dh",
    [MSG_DURATION_MINS]        = "%dm",
    [MSG_DURATION_H_M]         = "%dh %dm",
    [MSG_DURATION_D_H_M]       = "%dd %dh %dm",
};
