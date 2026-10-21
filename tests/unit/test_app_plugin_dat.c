/**
 * Unit tests for src/app/config/plugin_dat.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/config/plugin_dat.h"
#include "plugin/plugin_config.h"
#include "common/db_schema.h"

static void test_plugin_dat_roundtrip(void) {
    mock_pspsdk_reset();

    // Default load
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 1);
    ASSERT_EQ(plugin_dat_get_capture_psp(), 1);
    ASSERT_EQ(plugin_dat_get_capture_ps1(), 1);
    ASSERT_EQ(plugin_dat_get_capture_homebrew(), 1);
    ASSERT_EQ(plugin_dat_get_capture_psp_app(), 1);
    ASSERT_EQ(plugin_dat_get_capture_unknown(), 1);

    // Modify values
    plugin_dat_set_hotkey_enabled(1);
    plugin_dat_set_icon_enabled(0);
    plugin_dat_set_capture_psp(0);
    plugin_dat_set_capture_ps1(1);
    plugin_dat_set_capture_homebrew(0);
    plugin_dat_set_capture_psp_app(1);
    plugin_dat_set_capture_unknown(0);

    int res = plugin_dat_save();
    ASSERT_EQ(res, 0);

    // Verify raw file
    char path[160];
    snprintf(path, sizeof(path), "ms0:" GDIARY_BASE_DIR "/%s", PLUGIN_DAT);
    PluginConfigFile file;
    size_t rd = mock_vfs_read_file(path, &file, sizeof(file));
    ASSERT_EQ(rd, sizeof(file));
    ASSERT_EQ(file.version, PLUGIN_CONFIG_VERSION);
    ASSERT_EQ(file.hotkey_enabled, 1);
    ASSERT_EQ(file.icon_enabled, 0);
    ASSERT_EQ(file.capture_psp, 0);
    ASSERT_EQ(file.capture_ps1, 1);
    ASSERT_EQ(file.capture_homebrew, 0);
    ASSERT_EQ(file.capture_psp_app, 1);
    ASSERT_EQ(file.capture_unknown, 0);

    // Reset and reload
    plugin_dat_set_hotkey_enabled(0);
    plugin_dat_load();

    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 1);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_capture_psp(), 0);
    ASSERT_EQ(plugin_dat_get_capture_ps1(), 1);
    ASSERT_EQ(plugin_dat_get_capture_homebrew(), 0);
    ASSERT_EQ(plugin_dat_get_capture_psp_app(), 1);
    ASSERT_EQ(plugin_dat_get_capture_unknown(), 0);
}

static void test_plugin_dat_versions_and_edge_cases(void) {
    mock_pspsdk_reset();
    char path[160];
    snprintf(path, sizeof(path), "ms0:" GDIARY_BASE_DIR "/%s", PLUGIN_DAT);

    // 1. Corrupted / truncated file (< sizeof(PluginConfigFile))
    u8 small_buf[10] = { 0x01, 0x02, 0x03 };
    mock_vfs_map_file(path, small_buf, sizeof(small_buf));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 1);

    // 2. Unsupported version
    PluginConfigFile file;
    memset(&file, 0, sizeof(file));
    file.version = 999;
    file.hotkey_enabled = 1;
    mock_vfs_map_file(path, &file, sizeof(file));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 0); // Reset to default

    // 3. V1 file backwards compatibility
    memset(&file, 0, sizeof(file));
    file.version = PLUGIN_CONFIG_VERSION_V1;
    file.hotkey_enabled = 1;
    file.icon_enabled = 0; // V1 didn't have icon_enabled, should default to 1
    mock_vfs_map_file(path, &file, sizeof(file));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 1);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 1);
    ASSERT_EQ(plugin_dat_get_capture_psp(), 1);

    // 4. V2 file backwards compatibility
    memset(&file, 0, sizeof(file));
    file.version = PLUGIN_CONFIG_VERSION_V2;
    file.hotkey_enabled = 0;
    file.icon_enabled = 0;
    mock_vfs_map_file(path, &file, sizeof(file));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_capture_psp(), 1);

    // 5. V3 file backwards compatibility
    memset(&file, 0, sizeof(file));
    file.version = PLUGIN_CONFIG_VERSION_V3;
    file.hotkey_enabled = 1;
    file.icon_enabled = 1;
    file.capture_psp = 0;
    file.capture_ps1 = 0;
    file.capture_homebrew = 0;
    file.capture_psp_app = 0;
    file.capture_unknown = 0; // V3 didn't have unknown, should default to 1
    mock_vfs_map_file(path, &file, sizeof(file));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_capture_psp(), 0);
    ASSERT_EQ(plugin_dat_get_capture_ps1(), 0);
    ASSERT_EQ(plugin_dat_get_capture_homebrew(), 0);
    ASSERT_EQ(plugin_dat_get_capture_psp_app(), 0);
    ASSERT_EQ(plugin_dat_get_capture_unknown(), 1);

    // 6. Clamped flags (value > 1 falls back to default)
    memset(&file, 0, sizeof(file));
    file.version = PLUGIN_CONFIG_VERSION;
    file.hotkey_enabled = 55; // fallback 0
    file.icon_enabled = 55;   // fallback 1
    mock_vfs_map_file(path, &file, sizeof(file));
    plugin_dat_load();
    ASSERT_EQ(plugin_dat_get_hotkey_enabled(), 0);
    ASSERT_EQ(plugin_dat_get_icon_enabled(), 1);

    // 7. Save failure when open fails
    mock_vfs_fail_open(true);
    ASSERT_EQ(plugin_dat_save(), -1);
    mock_vfs_fail_open(false);
}

void run_test_app_plugin_dat(void) {
    TEST_SUITE_BEGIN("App Plugin Dat Module");
    TEST_RUN(test_plugin_dat_roundtrip);
    TEST_RUN(test_plugin_dat_versions_and_edge_cases);
    TEST_SUITE_END();
}
