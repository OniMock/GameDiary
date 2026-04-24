/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * Review by:
 * Semmelstulle
 * Date: 2026-04-24
 */

#include "app/i18n/i18n.h"
const char* g_lang_de_entries[MSG_COUNT] = {
    [MSG_MENU_STATS]           = "Statistiken",
    [MSG_MENU_GAMES]           = "Spiele",
    [MSG_MENU_SETTINGS]        = "Einstellungen",
    [MSG_MENU_ACTIVITY]        = "Aktivität",
    [MSG_STATS_TOTAL_PLAYTIME] = "Gesamtspielzeit",
    [MSG_STATS_SESSIONS]       = "Sitzungen",
    [MSG_STATS_LAST_PLAYED]    = "Zuletzt gespielt",
    [MSG_CTRL_BACK]            = "Zurück",
    [MSG_CTRL_SELECT]          = "Auswählen",
    [MSG_CTRL_MENU]            = "Menü",
    [MSG_CTRL_CONFIG]          = "Einstellungen",
    [MSG_SETTINGS_LANGUAGE]    = "Sprache",
    [MSG_SETTINGS_ABOUT]       = "Über",
    [MSG_SETTINGS_SUPPORT]     = "Support",
    [MSG_TOP_WEEK]             = "Woche",
    [MSG_TOP_MONTH]            = "Monat",
    [MSG_TOP_YEAR]             = "Jahr",
    [MSG_TOP_ALL]              = "Gesamt",
    [MSG_DETAILS_FIRST_PLAYED] = "Erstmals gespielt",
    [MSG_CAT_PSP]              = "PlayStation Portable",
    [MSG_CAT_PSX]              = "PlayStation (PSX)",
    [MSG_CAT_MINIS]            = "PlayStation Minis",
    [MSG_CAT_HOMEBREW]         = "Homebrew",
    [MSG_ERROR_NO_GAMES]       = "Keine Spiele gefunden",
    [MSG_CTRL_NAVIGATE]        = "Navigieren",

    [MSG_DAY_SUN]              = "So",
    [MSG_DAY_MON]              = "Mo",
    [MSG_DAY_TUE]              = "Di",
    [MSG_DAY_WED]              = "Mi",
    [MSG_DAY_THU]              = "Do",
    [MSG_DAY_FRI]              = "Fr",
    [MSG_DAY_SAT]              = "Sa",

    // Format Time
    [MSG_STATS_NO_ACTIVITY]    = "Keine kürzliche Aktivität",
    [MSG_STATS_DAYS_ACTIVE]    = "Aktive Tage",
    [MSG_STATS_NEVER]          = "Nie",
    [MSG_DATE_FORMAT]          = "%d.%m.%Y",
    [MSG_DATE_FORMAT_SHORT]    = "%d.%m",
    [MSG_DURATION_DAYS]        = "%dt",
    [MSG_DURATION_HOURS]       = "%dh",
    [MSG_DURATION_MINS]        = "%dm",
    [MSG_DURATION_H_M]         = "%dh %dm",
    [MSG_DURATION_D_H_M]       = "%dt %dh %dm",

    [MSG_STATS_MODE_WEEKLY]    = "Wöchentlich",
    [MSG_STATS_MODE_MONTHLY]   = "Monatlich",
    [MSG_STATS_MODE_YEARLY]    = "Jährlich",

    [MSG_MONTH_JAN]            = "Januar",
    [MSG_MONTH_FEB]            = "Februar",
    [MSG_MONTH_MAR]            = "März",
    [MSG_MONTH_APR]            = "April",
    [MSG_MONTH_MAY]            = "Mai",
    [MSG_MONTH_JUN]            = "Juni",
    [MSG_MONTH_JUL]            = "Juli",
    [MSG_MONTH_AUG]            = "August",
    [MSG_MONTH_SEP]            = "September",
    [MSG_MONTH_OCT]            = "Oktober",
    [MSG_MONTH_NOV]            = "November",
    [MSG_MONTH_DEC]            = "Dezember",

    [MSG_FILTER]               = "Filter",

    [MSG_HINT_HELPER]              = "[ L ]: Hilfe",
    [MSG_HELP_TITLE]               = "Hilfe",
    [MSG_HELP_BTN_X_SELECT]        = "[ X ]: Auswählen",
    [MSG_HELP_BTN_X_CONFIRM]       = "[ X ]: Bestätigen",
    [MSG_HELP_BTN_X_CHANGE]        = "[ X ]: Ändern",
    [MSG_HELP_BTN_O_BACK]          = "[ O ]: Zurück",
    [MSG_HELP_BTN_START_MENU]      = "[ START ]: Menü",
    [MSG_HELP_BTN_SELECT_CONFIG]   = "[ SELECT ]: Einstellungen",
    [MSG_HELP_BTN_ARROWS_NAVIGATE] = "[ ↑ ↓ ]: Navigieren",
    [MSG_HELP_BTN_ANALOG_NAVIGATE] = "[ ← → ] oder [ ◉ ]: Navigieren",
    [MSG_HELP_BTN_ANALOG_FILTER]   = "[ ← → ] oder [ ◉ ]: Filter ändern",
    [MSG_HELP_BTN_SQUARE_FILTER]   = "[ ■ ]: Filter",
    [MSG_HELP_BTN_TRIANGLE_STATS]  = "[ △ ]: Statistiken",
    [MSG_HELP_CONTROLS]            = "Steuerung:",
    [MSG_HELP_INFO_LABEL]          = "Info:",
    [MSG_HELP_DESC_GAMES]          = "Hier kannst du nach Kategorien filtern, die Gesamtspielzeit anzeigen und letzte Sitzungen pro Spiel verfolgen.",
    [MSG_HELP_DESC_STATS]          = "Analysiere deine Spielzeit mit detaillierten Diagrammen. Wechsle zwischen wöchentlich, monatlich und jährlich, um deine Gewohnheiten zu verstehen.",
    [MSG_HELP_DESC_ACTIVITY]       = "Chronologische Übersicht deiner letzten Sitzungen. Sieh genau, wann und wie lange du gespielt hast.",
    [MSG_HELP_DESC_SETTINGS]       = "Passe die Sprache und andere Einstellungen an.",
    [MSG_HELP_DESC_DETAILS]        = "Detaillierte Infos zu einem Spiel. Zeigt den Sitzungsverlauf, das Datum, an dem du das Spiel zum ersten und letzten Mal gespielt hast, und deine Gesamtspielzeiten an.",
    [MSG_HELP_DESC_MAIN_MENU]      = "Hauptmenü zur Navigation zwischen Spielen, Statistiken, Aktivität und Einstellungen.",
    [MSG_HELP_DESC_LANG_SELECT]    = "Wähle die Sprache aus. Änderungen werden sofort übernommen.",
    [MSG_HELP_DESC_ABOUT]          = "Informationen über die Anwendung, Version und Entwickler.",
    [MSG_HELP_DESC_SUPPORT]        = "Wie du den Entwickler unterstützen kannst.",
    [MSG_HELP_CLOSE_HINT]          = "[ X / O ]: Schließen",

    [MSG_ABOUT_TITLE]              = "Über %s",
    [MSG_ABOUT_DESC]               = "%s ist ein Spielzeit-Tracker für die PSP, der dir hilft, deine Bibliothek zu verwalten.",
    [MSG_ABOUT_GITHUB]             = "github.com/OniMock/%s",
    [MSG_ABOUT_VERSION]            = "Version",
    [MSG_ABOUT_PSP_SDK]            = "PSP SDK",
    [MSG_ABOUT_DATE]               = "Datum",
    [MSG_ABOUT_DEVELOPER]          = "Entwickelt von OniMock",
    [MSG_SUPPORT_DESC]             = "Wenn dir meine Arbeit gefällt und du das Projekt unterstützen möchtest, kannst du eine Spende in Betracht ziehen.",
    [MSG_SUPPORT_COFFEE]           = "Buy me a Coffee",
    [MSG_SUPPORT_WALLET]           = "Wallet EVM",
};
