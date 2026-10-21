/**
 * Unit tests for src/app/data/game_category.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/data/game_category.h"
#include "app/i18n/i18n.h"

static void test_category_normalization(void) {
    ASSERT_EQ(game_category_normalize(CAT_PSP), CAT_PSP);
    ASSERT_EQ(game_category_normalize(CAT_PS1), CAT_PS1);
    ASSERT_EQ(game_category_normalize(CAT_HOMEBREW), CAT_HOMEBREW);
    ASSERT_EQ(game_category_normalize(CAT_VSH), CAT_PSP); // VSH maps to PSP
    ASSERT_EQ(game_category_normalize(CAT_UNKNOWN), CAT_UNKNOWN);
    ASSERT_EQ(game_category_normalize(10), CAT_UNKNOWN);
    ASSERT_EQ(game_category_normalize(255), CAT_UNKNOWN);
}

static void test_category_names(void) {
    i18n_set_language(LANG_EN);

    const char *n_psp = game_category_get_name(CAT_PSP);
    const char *n_ps1 = game_category_get_name(CAT_PS1);
    const char *n_hb  = game_category_get_name(CAT_HOMEBREW);
    const char *n_vsh = game_category_get_name(CAT_VSH);
    const char *n_unk = game_category_get_name(CAT_UNKNOWN);

    ASSERT_NOT_NULL(n_psp);
    ASSERT_NOT_NULL(n_ps1);
    ASSERT_NOT_NULL(n_hb);
    ASSERT_NOT_NULL(n_vsh);
    ASSERT_NOT_NULL(n_unk);

    // VSH name should equal PSP name due to normalization
    ASSERT_STR_EQ(n_vsh, n_psp);
}

void run_test_game_category(void) {
    TEST_SUITE_BEGIN("Game Category Module");
    TEST_RUN(test_category_normalization);
    TEST_RUN(test_category_names);
    TEST_SUITE_END();
}
