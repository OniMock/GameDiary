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

void run_test_app_plugin_dat(void) {
    TEST_SUITE_BEGIN("App Plugin Dat Module");
    TEST_RUN(test_plugin_dat_roundtrip);
    TEST_SUITE_END();
}
