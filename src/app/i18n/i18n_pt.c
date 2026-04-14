#include "app/i18n.h"

// Using ISO-8859-1 escape sequences to ensure compatibility with ltn8.pgf
// ê = \xea, ç = \xe7, á = \xe1, ã = \xe3, õ = \xf5, ó = \xf3, í = \xed
static const TranslationEntry pt_entries[] = {
    {0, "Di\xe1rio de Jogo"},
    {0, "In\xedcio"},
    {0, "Estat\xedsticas"},
    {0, "Jogos"},
    {0, "Ajustes"},
    {0, "Tempo Total"},
    {0, "Sess\xf5es"},
    {0, "\xdaltimo Jogo"},
    {0, "Pressione X para os jogos"},
    {0, "Voltar"},
    {0, "Selecionar"},
    {0, "Idioma"},
    {0, "Semana"},
    {0, "M\xeas"},
    {0, "Sempre"}
};

LanguagePack g_lang_pt = {
    "pt",
    "Portugu\xeas",
    pt_entries,
    sizeof(pt_entries) / sizeof(TranslationEntry)
};
