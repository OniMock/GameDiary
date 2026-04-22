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
  * @file i18n.c
  * @brief Internationalization system implementation.
  */

#include "app/i18n/i18n.h"
#include <psputility.h>
#include <stddef.h>

/* --- External Language Arrays --- */
extern const char *g_lang_en_entries[MSG_COUNT];
extern const char *g_lang_pt_entries[MSG_COUNT];
extern const char *g_lang_es_entries[MSG_COUNT];
extern const char *g_lang_ru_entries[MSG_COUNT];
extern const char *g_lang_jp_entries[MSG_COUNT];
extern const char *g_lang_cn_entries[MSG_COUNT];

/* --- Global Pointer --- */
const char **g_i18n_msg = NULL;

/* --- Language Pack Registry --- */
typedef struct {
  const char *name;
  const char **entries;
  const ImageResource *flag;
} LanguageRegistry;

const ImageResource* i18n_get_current_flag(void);
const ImageResource* i18n_get_lang_flag(int index);

static const LanguageRegistry g_lang_registry[LANG_COUNT] = {
    [LANG_EN] = {"English", g_lang_en_entries, &GD_IMG_FLAG_EN_PNG},
    [LANG_PT] = {"Português", g_lang_pt_entries, &GD_IMG_FLAG_PT_PNG},
    [LANG_ES] = {"Español", g_lang_es_entries, &GD_IMG_FLAG_ES_PNG},
    [LANG_RU] = {"Русский", g_lang_ru_entries, &GD_IMG_FLAG_RU_PNG},
    [LANG_JP] = {"日本語", g_lang_jp_entries, &GD_IMG_FLAG_JP_PNG},
    [LANG_CN] = {"中文", g_lang_cn_entries, &GD_IMG_FLAG_CN_PNG}};

static int g_current_lang_idx = LANG_EN;

const ImageResource* i18n_get_current_flag(void) {
    return g_lang_registry[g_current_lang_idx].flag;
}

const ImageResource* i18n_get_lang_flag(int index) {
    if (index < 0 || index >= LANG_COUNT) return NULL;
    return g_lang_registry[index].flag;
}

/* --- Private Functions --- */

static int i18n_detect_system_lang(void) {
  int lang = 1; // Default to English (1)
  if (sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang) < 0) {
    return LANG_EN;
  }

  switch (lang) {
  case PSP_SYSTEMPARAM_LANGUAGE_PORTUGUESE:
    return LANG_PT;
  case PSP_SYSTEMPARAM_LANGUAGE_SPANISH:
    return LANG_ES;
  case PSP_SYSTEMPARAM_LANGUAGE_RUSSIAN:
    return LANG_RU;
  case PSP_SYSTEMPARAM_LANGUAGE_JAPANESE:
    return LANG_JP;
  case PSP_SYSTEMPARAM_LANGUAGE_CHINESE_TRADITIONAL:
    return LANG_CN;
  case PSP_SYSTEMPARAM_LANGUAGE_ENGLISH:
  default:
    return LANG_EN;
  }
}

/* --- Public API --- */

void i18n_init(int force_lang) {
  int target_lang = force_lang;

  if (target_lang == LANG_AUTO) {
    target_lang = i18n_detect_system_lang();
  }

  // Bounds check
  if (target_lang < 0 || target_lang >= LANG_COUNT) {
    target_lang = LANG_EN;
  }

  i18n_set_language(target_lang);
}

void i18n_set_language(int lang_index) {
  if (lang_index < 0 || lang_index >= LANG_COUNT) {
    lang_index = LANG_EN;
  }

  g_current_lang_idx = lang_index;
  g_i18n_msg = g_lang_registry[lang_index].entries;
}

const char *i18n_get(MessageId id) {
  if (id < 0 || id >= MSG_COUNT || !g_i18n_msg) {
    return "<?>";
  }
  return g_i18n_msg[id];
}

int i18n_current_lang(void) { return g_current_lang_idx; }

const char *i18n_get_lang_name(int index) {
  if (index < 0 || index >= LANG_COUNT) {
    return "Unknown";
  }
  return g_lang_registry[index].name;
}
