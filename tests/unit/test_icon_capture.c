/**
 * Unit tests for src/plugin/icon_capture.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/icon_capture.h"
#include "common/models.h"
#include <string.h>

static void test_icon_capture_psp_category_disc0(void) {
    mock_pspsdk_reset();

    const char *png_data = "\x89PNG\r\n\x1a\nFakeIconData";
    mock_vfs_map_file("disc0:/PSP_GAME/ICON0.PNG", png_data, 18);

    const char *dest_dir = "ms0:/PSP/COMMON/GameDiary/icons";
    utils_capture_icon("ULUS10001", CAT_PSP, dest_dir, NULL);

    // Verify destination exists and contains the PNG data
    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/icons/ULUS10001.png"));
    char buf[32] = {0};
    size_t rd = mock_vfs_read_file("ms0:/PSP/COMMON/GameDiary/icons/ULUS10001.png", buf, sizeof(buf));
    ASSERT_EQ(rd, 18);
    ASSERT_EQ(memcmp(buf, png_data, 18), 0);
}

static void test_icon_capture_sidecar_uppercase(void) {
    mock_pspsdk_reset();

    const char *png_data = "\x89PNG\r\n\x1a\nSidecarUpper";
    mock_vfs_map_file("ms0:/PSP/GAME/MyHomebrew/ICON0.PNG", png_data, 18);

    const char *dest_dir = "ms0:/PSP/COMMON/GameDiary/icons";
    const char *exec_path = "ms0:/PSP/GAME/MyHomebrew/EBOOT.PBP";
    utils_capture_icon("HBX12345678", CAT_HOMEBREW, dest_dir, exec_path);

    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/icons/HBX12345678.png"));
    char buf[32] = {0};
    size_t rd = mock_vfs_read_file("ms0:/PSP/COMMON/GameDiary/icons/HBX12345678.png", buf, sizeof(buf));
    ASSERT_EQ(rd, 18);
    ASSERT_EQ(memcmp(buf, png_data, 18), 0);
}

static void test_icon_capture_sidecar_lowercase(void) {
    mock_pspsdk_reset();

    const char *png_data = "\x89PNG\r\n\x1a\nSidecarLower";
    mock_vfs_map_file("ms0:/PSP/GAME/MyHomebrew/icon0.png", png_data, 18);

    const char *dest_dir = "ms0:/PSP/COMMON/GameDiary/icons";
    const char *exec_path = "ms0:/PSP/GAME/MyHomebrew/EBOOT.PBP";
    utils_capture_icon("HBX87654321", CAT_HOMEBREW, dest_dir, exec_path);

    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/icons/HBX87654321.png"));
    char buf[32] = {0};
    size_t rd = mock_vfs_read_file("ms0:/PSP/COMMON/GameDiary/icons/HBX87654321.png", buf, sizeof(buf));
    ASSERT_EQ(rd, 18);
    ASSERT_EQ(memcmp(buf, png_data, 18), 0);
}

static void test_icon_capture_none_available(void) {
    mock_pspsdk_reset();

    const char *dest_dir = "ms0:/PSP/COMMON/GameDiary/icons";
    const char *exec_path = "ms0:/PSP/GAME/NoIconApp/EBOOT.PBP";
    utils_capture_icon("UNKNOWN-00000", CAT_HOMEBREW, dest_dir, exec_path);

    // Destination should NOT be created
    ASSERT_FALSE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/icons/UNKNOWN-00000.png"));
}

void run_icon_capture_tests(void) {
    TEST_RUN(test_icon_capture_psp_category_disc0);
    TEST_RUN(test_icon_capture_sidecar_uppercase);
    TEST_RUN(test_icon_capture_sidecar_lowercase);
    TEST_RUN(test_icon_capture_none_available);
}
