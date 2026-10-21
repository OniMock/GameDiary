/**
 * Unit tests for src/app/i18n/i18n.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/i18n/i18n.h"

static void test_i18n_switching_and_retrieval(void) {
    mock_pspsdk_reset();

    // English
    i18n_set_language(LANG_EN);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);
    ASSERT_STR_EQ(i18n_get_lang_name(LANG_EN), "English");

    const char *en_games = i18n_get(MSG_MENU_GAMES);
    ASSERT_NOT_NULL(en_games);
    ASSERT_STR_EQ(en_games, "Games");

    // Portuguese
    i18n_set_language(LANG_PT);
    ASSERT_EQ(i18n_current_lang(), LANG_PT);
    ASSERT_STR_EQ(i18n_get_lang_name(LANG_PT), "Portuguese");

    const char *pt_games = i18n_get(MSG_MENU_GAMES);
    ASSERT_NOT_NULL(pt_games);
    ASSERT_STR_EQ(pt_games, "Jogos");

    // Spanish
    i18n_set_language(LANG_ES);
    ASSERT_EQ(i18n_current_lang(), LANG_ES);
    const char *es_games = i18n_get(MSG_MENU_GAMES);
    ASSERT_NOT_NULL(es_games);
    ASSERT_STR_EQ(es_games, "Juegos");

    // Invalid message ID handling
    ASSERT_STR_EQ(i18n_get((MessageId)-1), "<?>");
    ASSERT_STR_EQ(i18n_get((MessageId)9999), "<?>");
}

static void test_i18n_auto_detect_psp_system(void) {
    mock_pspsdk_reset();

    // Mock PSP firmware language: Japanese (0)
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_JAPANESE);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_JP);

    // Mock PSP firmware language: French (2)
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_FRENCH);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_FR);

    // Mock PSP firmware language: Portuguese (7)
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_PORTUGUESE);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_PT);
}

static void test_i18n_sorted_indices(void) {
    // Check sorted index mapping bounds
    ASSERT_EQ(i18n_get_sorted_lang_index(0), LANG_CN);
    ASSERT_EQ(i18n_get_sorted_lang_index(-1), LANG_EN);
    ASSERT_EQ(i18n_get_sorted_lang_index(100), LANG_EN);
}

void run_test_i18n(void) {
    TEST_SUITE_BEGIN("Internationalization (i18n) Module");
    TEST_RUN(test_i18n_switching_and_retrieval);
    TEST_RUN(test_i18n_auto_detect_psp_system);
    TEST_RUN(test_i18n_sorted_indices);
    TEST_SUITE_END();
}
