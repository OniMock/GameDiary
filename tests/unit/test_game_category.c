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

#include "app/data/data_loader.h"
#include "common/storage.h"

static void test_category_get_available(void) {
    mock_pspsdk_reset();
    const char *k_base_dir = "ms0:" GDIARY_BASE_DIR;
    storage_init(k_base_dir);

    GameMetadata m1 = { .category = CAT_PSP, .game_id = "ULES-00123" };
    GameMetadata m2 = { .category = CAT_PS1, .game_id = "SLUS-00001" };
    GameMetadata m3 = { .category = CAT_HOMEBREW, .game_id = "HBX-12345" };

    u32 uid1 = 0, uid2 = 0, uid3 = 0;
    storage_get_or_create_game(&m1, &uid1);
    storage_get_or_create_game(&m2, &uid2);
    storage_get_or_create_game(&m3, &uid3);

    data_load_all();

    u8 available_cats[CAT_UNKNOWN + 1];
    memset(available_cats, 0xFF, sizeof(available_cats));
    int count = game_category_get_available(available_cats);
    ASSERT_EQ(count, 3);

    int has_psp = 0, has_ps1 = 0, has_hb = 0;
    for (int i = 0; i < count; i++) {
        if (available_cats[i] == CAT_PSP) has_psp = 1;
        if (available_cats[i] == CAT_PS1) has_ps1 = 1;
        if (available_cats[i] == CAT_HOMEBREW) has_hb = 1;
    }
    ASSERT_TRUE(has_psp);
    ASSERT_TRUE(has_ps1);
    ASSERT_TRUE(has_hb);
}

void run_test_game_category(void) {
    TEST_SUITE_BEGIN("Game Category Module");
    TEST_RUN(test_category_normalization);
    TEST_RUN(test_category_names);
    TEST_RUN(test_category_get_available);
    TEST_SUITE_END();
}

