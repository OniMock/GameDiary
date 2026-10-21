/**
 * Unit tests for src/plugin/plugin_config.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/plugin_config.h"
#include "common/db_schema.h"

static void test_plugin_config_missing_file(void) {
    mock_pspsdk_reset();
    plugin_config_init();

    const PluginConfigRuntime *cfg = plugin_config_get();
    ASSERT_NOT_NULL(cfg);
    ASSERT_EQ(cfg->hotkey_enabled, 0);
    ASSERT_EQ(cfg->icon_enabled, 1);
    ASSERT_EQ(cfg->capture_psp, 1);
    ASSERT_EQ(cfg->capture_ps1, 1);
    ASSERT_EQ(cfg->capture_homebrew, 1);
    ASSERT_EQ(cfg->capture_psp_app, 1);
    ASSERT_EQ(cfg->capture_unknown, 1);
}

static void test_plugin_config_v4_valid(void) {
    mock_pspsdk_reset();

    PluginConfigFile file;
    memset(&file, 0, sizeof(file));
    file.version = 4;
    file.hotkey_enabled = 1;
    file.icon_enabled = 0;
    file.capture_psp = 1;
    file.capture_ps1 = 0;
    file.capture_homebrew = 1;
    file.capture_psp_app = 0;
    file.capture_unknown = 0;

    char path[160];
    snprintf(path, sizeof(path), "ms0:" GDIARY_BASE_DIR "/%s", PLUGIN_DAT);
    mock_vfs_map_file(path, &file, sizeof(file));

    plugin_config_init();

    const PluginConfigRuntime *cfg = plugin_config_get();
    ASSERT_EQ(cfg->hotkey_enabled, 1);
    ASSERT_EQ(cfg->icon_enabled, 0);
    ASSERT_EQ(cfg->capture_psp, 1);
    ASSERT_EQ(cfg->capture_ps1, 0);
    ASSERT_EQ(cfg->capture_homebrew, 1);
    ASSERT_EQ(cfg->capture_psp_app, 0);
    ASSERT_EQ(cfg->capture_unknown, 0);
}

static void test_plugin_config_backward_compatibility(void) {
    mock_pspsdk_reset();

    char path[160];
    snprintf(path, sizeof(path), "ms0:" GDIARY_BASE_DIR "/%s", PLUGIN_DAT);

    // v1: icon_enabled and capture flags absent in v1, should default to enabled
    PluginConfigFile f1;
    memset(&f1, 0, sizeof(f1));
    f1.version = 1;
    f1.hotkey_enabled = 1;
    f1.icon_enabled = 0; // In v1 this field should be ignored
    mock_vfs_map_file(path, &f1, sizeof(f1));

    plugin_config_init();
    const PluginConfigRuntime *cfg1 = plugin_config_get();
    ASSERT_EQ(cfg1->hotkey_enabled, 1);
    ASSERT_EQ(cfg1->icon_enabled, 1); // ignored in v1 -> forced 1
    ASSERT_EQ(cfg1->capture_psp, 1);

    // v2: reads icon_enabled, but capture flags should default to 1
    PluginConfigFile f2;
    memset(&f2, 0, sizeof(f2));
    f2.version = 2;
    f2.hotkey_enabled = 0;
    f2.icon_enabled = 0;
    mock_vfs_map_file(path, &f2, sizeof(f2));

    plugin_config_init();
    const PluginConfigRuntime *cfg2 = plugin_config_get();
    ASSERT_EQ(cfg2->hotkey_enabled, 0);
    ASSERT_EQ(cfg2->icon_enabled, 0);
    ASSERT_EQ(cfg2->capture_psp, 1);

    // Unsupported version (e.g. 99)
    PluginConfigFile f_bad;
    memset(&f_bad, 0, sizeof(f_bad));
    f_bad.version = 99;
    mock_vfs_map_file(path, &f_bad, sizeof(f_bad));

    plugin_config_init();
    const PluginConfigRuntime *cfg_bad = plugin_config_get();
    ASSERT_EQ(cfg_bad->hotkey_enabled, 0); // defaults
    ASSERT_EQ(cfg_bad->icon_enabled, 1);
}

static void test_plugin_config_should_capture(void) {
    mock_pspsdk_reset();

    PluginConfigFile file;
    memset(&file, 0, sizeof(file));
    file.version = 4;
    file.hotkey_enabled = 0;
    file.icon_enabled = 1;
    file.capture_psp = 1;
    file.capture_ps1 = 0;
    file.capture_homebrew = 1;
    file.capture_psp_app = 0;
    file.capture_unknown = 0;

    char path[160];
    snprintf(path, sizeof(path), "ms0:" GDIARY_BASE_DIR "/%s", PLUGIN_DAT);
    mock_vfs_map_file(path, &file, sizeof(file));

    plugin_config_init();

    GameMetadata m_psp = { .category = CAT_PSP, .game_id = "ULUS10041" };
    ASSERT_EQ(plugin_config_should_capture(&m_psp), 1);

    GameMetadata m_ps1 = { .category = CAT_PS1, .game_id = "SLUS00067" };
    ASSERT_EQ(plugin_config_should_capture(&m_ps1), 0);

    GameMetadata m_hb = { .category = CAT_HOMEBREW, .game_id = "HBX12345678", .file_path = "ms0:/PSP/GAME/DOOM/EBOOT.PBP" };
    ASSERT_EQ(plugin_config_should_capture(&m_hb), 1);

    GameMetadata m_app = { .category = CAT_HOMEBREW, .game_id = "NPIA00013", .file_path = "ms0:/PSP/APP/NPIA00013/EBOOT.PBP" };
    ASSERT_EQ(plugin_config_should_capture(&m_app), 0);

    GameMetadata m_unk = { .category = CAT_UNKNOWN, .game_id = "UNKNOWN-00000" };
    ASSERT_EQ(plugin_config_should_capture(&m_unk), 0);

    // NULL pointer should return 1 (capture)
    ASSERT_EQ(plugin_config_should_capture(NULL), 1);
}

void run_test_plugin_config(void) {
    TEST_SUITE_BEGIN("Plugin Config Module");
    TEST_RUN(test_plugin_config_missing_file);
    TEST_RUN(test_plugin_config_v4_valid);
    TEST_RUN(test_plugin_config_backward_compatibility);
    TEST_RUN(test_plugin_config_should_capture);
    TEST_SUITE_END();
}
