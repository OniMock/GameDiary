#include "app/i18n.h"

static const TranslationEntry en_entries[] = {
    {0, "Game Diary"},
    {0, "Dashboard"},
    {0, "Statistics"},
    {0, "Games"},
    {0, "Settings"},
    {0, "Total Playtime"},
    {0, "Sessions"},
    {0, "Last Played"},
    {0, "Press X to view games"},
    {0, "Back"},
    {0, "Select"},
    {0, "Language"},
    {0, "Week"},
    {0, "Month"},
    {0, "All Time"}
};

LanguagePack g_lang_en = {
    "en",
    "English",
    en_entries,
    sizeof(en_entries) / sizeof(TranslationEntry)
};
