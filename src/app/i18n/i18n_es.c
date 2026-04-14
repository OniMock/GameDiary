#include "app/i18n.h"

// Using ISO-8859-1 escape sequences to ensure compatibility with ltn8.pgf
// í = \xed, ñ = \xf1, á = \xe1, ó = \xf3, ú = \xfa
static const TranslationEntry es_entries[] = {
    {0, "Diario de Juego"},
    {0, "Inicio"},
    {0, "Estad\xedsticas"},
    {0, "Juegos"},
    {0, "Ajustes"},
    {0, "Tiempo Total"},
    {0, "Sesiones"},
    {0, "\xdaltimo Juego"},
    {0, "Presione X para ver juegos"},
    {0, "Volver"},
    {0, "Seleccionar"},
    {0, "Idioma"},
    {0, "Semana"},
    {0, "Mes"},
    {0, "Siempre"}
};

LanguagePack g_lang_es = {
    "es",
    "Espa\xf1ol",
    es_entries,
    sizeof(es_entries) / sizeof(TranslationEntry)
};
