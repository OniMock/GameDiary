/**
 * Unit tests for src/app/config/config.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/config/config.h"
#include "common/db_schema.h"

static void test_config_missing_file_defaults(void) {
    mock_pspsdk_reset();
    config_init("ms0:/PSP/GAME/GameDiary/EBOOT.PBP");

    int res = config_load();
    ASSERT_EQ(res, 0);

    AppConfig *cfg = config_get();
    ASSERT_NOT_NULL(cfg);
    ASSERT_EQ(cfg->language, -1);
    ASSERT_EQ(cfg->sfx_enabled, 1);
    ASSERT_EQ(cfg->theme, 0);
    ASSERT_EQ(cfg->format_hours_only, 0);

    // Verify file was written to disk
    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/GAME/GameDiary/config.dat"));
}

static void test_config_save_and_reload(void) {
    mock_pspsdk_reset();
    config_init("ms0:/PSP/GAME/GameDiary/EBOOT.PBP");
    config_load();

    AppConfig *cfg = config_get();
    cfg->language = 2; // LANG_ES
    cfg->sfx_enabled = 0;
    cfg->theme = 1;
    cfg->format_hours_only = 1;

    int res = config_write_file(cfg);
    ASSERT_EQ(res, 0);

    // Reset memory and reload
    memset(cfg, 0, sizeof(AppConfig));
    res = config_load();
    ASSERT_EQ(res, 0);

    ASSERT_EQ(cfg->language, 2);
    ASSERT_EQ(cfg->sfx_enabled, 0);
    ASSERT_EQ(cfg->theme, 1);
    ASSERT_EQ(cfg->format_hours_only, 1);
}

static void test_config_legacy_migration(void) {
    mock_pspsdk_reset();
    config_init("ms0:/PSP/GAME/GameDiary/EBOOT.PBP");

    // Write 8-byte legacy config (language=1, sfx=0)
    int legacy_8[2] = { 1, 0 };
    mock_vfs_map_file("ms0:/PSP/GAME/GameDiary/config.dat", legacy_8, sizeof(legacy_8));

    int res = config_load();
    ASSERT_EQ(res, 0);

    AppConfig *cfg = config_get();
    ASSERT_EQ(cfg->language, 1);
    ASSERT_EQ(cfg->sfx_enabled, 0);
    ASSERT_EQ(cfg->theme, 0); // Default migrated
    ASSERT_EQ(cfg->format_hours_only, 0); // Default migrated

    // Write 12-byte legacy config (language=3, sfx=1, theme=1)
    int legacy_12[3] = { 3, 1, 1 };
    mock_vfs_map_file("ms0:/PSP/GAME/GameDiary/config.dat", legacy_12, sizeof(legacy_12));

    res = config_load();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(cfg->language, 3);
    ASSERT_EQ(cfg->sfx_enabled, 1);
    ASSERT_EQ(cfg->theme, 1);
    ASSERT_EQ(cfg->format_hours_only, 0);
}

void run_test_app_config(void) {
    TEST_SUITE_BEGIN("App Configuration Module");
    TEST_RUN(test_config_missing_file_defaults);
    TEST_RUN(test_config_save_and_reload);
    TEST_RUN(test_config_legacy_migration);
    TEST_SUITE_END();
}
