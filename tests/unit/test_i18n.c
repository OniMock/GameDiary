/**
 * Unit tests for src/app/i18n/i18n.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/i18n/i18n.h"

extern const char **g_i18n_msg;

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

    // NULL message table check
    const char **saved_msg = g_i18n_msg;
    g_i18n_msg = NULL;
    ASSERT_STR_EQ(i18n_get(MSG_MENU_GAMES), "<?>");

    // Fallback to English when string is missing in table
    const char *mock_incomplete[MSG_COUNT];
    memset(mock_incomplete, 0, sizeof(mock_incomplete));
    g_i18n_msg = mock_incomplete;
    ASSERT_STR_EQ(i18n_get(MSG_MENU_GAMES), "Games");

    g_i18n_msg = saved_msg;
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

    // Spanish
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_SPANISH);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_ES);

    // Russian
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_RUSSIAN);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_RU);

    // Chinese Traditional
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_CHINESE_TRADITIONAL);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_CN);

    // German
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_GERMAN);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_DE);

    // Italian
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_ITALIAN);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_IT);

    // English
    mock_hardware_set_system_language(PSP_SYSTEMPARAM_LANGUAGE_ENGLISH);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    // Unknown language code -> fallback to English
    mock_hardware_set_system_language(999);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    // sceUtilityGetSystemParamInt failure -> fallback to English
    mock_hardware_set_system_language(-1);
    i18n_switch_language(LANG_AUTO);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);
}

static void test_i18n_flags_and_init(void) {
    mock_pspsdk_reset();

    // Initialize with German
    i18n_init(LANG_DE);
    ASSERT_EQ(i18n_current_lang(), LANG_DE);

    // Current flag should be valid
    const ImageResource *flag = i18n_get_current_flag();
    ASSERT_NOT_NULL(flag);

    // Specific language flags
    ASSERT_NOT_NULL(i18n_get_lang_flag(LANG_EN));
    ASSERT_NOT_NULL(i18n_get_lang_flag(LANG_CN));
    ASSERT_NOT_NULL(i18n_get_lang_flag(LANG_JP));

    // Bounds checking on flags
    ASSERT_NULL(i18n_get_lang_flag(-1));
    ASSERT_NULL(i18n_get_lang_flag(LANG_COUNT));
    ASSERT_NULL(i18n_get_lang_flag(999));

    // Font reload call
    i18n_reload_font();
}

static void test_i18n_bounds_and_unknowns(void) {
    mock_pspsdk_reset();

    // Out of bounds switch -> defaults to English
    i18n_switch_language(-5);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    i18n_switch_language(100);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    // Out of bounds set -> defaults to English
    i18n_set_language(-1);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    i18n_set_language(999);
    ASSERT_EQ(i18n_current_lang(), LANG_EN);

    // Out of bounds language name -> returns "Unknown"
    ASSERT_STR_EQ(i18n_get_lang_name(-1), "Unknown");
    ASSERT_STR_EQ(i18n_get_lang_name(LANG_COUNT), "Unknown");
    ASSERT_STR_EQ(i18n_get_lang_name(999), "Unknown");
}

static void test_i18n_sorted_indices(void) {
    // Check sorted index mapping bounds
    ASSERT_EQ(i18n_get_sorted_lang_index(0), LANG_CN);
    ASSERT_EQ(i18n_get_sorted_lang_index(-1), LANG_EN);
    ASSERT_EQ(i18n_get_sorted_lang_index(100), LANG_EN);
}

static void test_i18n_alphabetical_order(void) {
    /* BUG-05 Verification:
     * In src/app/i18n/i18n.c:
     * g_lang_sorted_order is documented as:
     * "Sorted alphabetically by language name:
     *   0 Chinese  (LANG_CN)
     *   1 English  (LANG_EN)
     *   2 French   (LANG_FR)
     *   3 German   (LANG_DE) ... "
     * In the English alphabet, "French" comes before "German".
     * Therefore, index 2 ("French") and index 3 ("German") must be strictly alphabetical. */
    const char *name_2 = i18n_get_lang_name(i18n_get_sorted_lang_index(2));
    const char *name_3 = i18n_get_lang_name(i18n_get_sorted_lang_index(3));
    ASSERT_STR_EQ(name_2, "French");
    ASSERT_STR_EQ(name_3, "German");

    /* Strict A-Z assertion: name_2 must be lexicographically before name_3. */
    ASSERT_TRUE(strcmp(name_2, name_3) < 0);
}

void run_test_i18n(void) {
    TEST_SUITE_BEGIN("Internationalization (i18n) Module");
    TEST_RUN(test_i18n_switching_and_retrieval);
    TEST_RUN(test_i18n_auto_detect_psp_system);
    TEST_RUN(test_i18n_flags_and_init);
    TEST_RUN(test_i18n_bounds_and_unknowns);
    TEST_RUN(test_i18n_sorted_indices);
    TEST_RUN(test_i18n_alphabetical_order);
    TEST_SUITE_END();
}
