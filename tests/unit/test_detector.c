/**
 * Unit tests for src/plugin/detector.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/detector.h"
#include "common/models.h"
#include <string.h>

static void test_detector_init_with_system_control(void) {
    mock_pspsdk_reset();

    const char *disc_id = "ULUS10041";
    const char *title = "Test Game PSP";
    mock_cfw_set_init_param("DISC_ID", disc_id, strlen(disc_id));
    mock_cfw_set_init_param("TITLE", title, strlen(title));
    mock_cfw_set_apitype(0x120); // ISO/UMD
    mock_cfw_set_executable_path("ms0:/ISO/TestGame.iso");

    detector_init();

    const GameMetadata *meta = detector_get_metadata();
    ASSERT_TRUE(meta != NULL);
    ASSERT_STR_EQ(meta->game_id, "ULUS10041");
    ASSERT_STR_EQ(meta->game_name, "Test Game PSP");
}

static void test_detector_init_late_when_already_known(void) {
    mock_pspsdk_reset();

    const char *disc_id = "UCUS98701";
    const char *title = "Known Game";
    mock_cfw_set_init_param("DISC_ID", disc_id, strlen(disc_id));
    mock_cfw_set_init_param("TITLE", title, strlen(title));
    mock_cfw_set_apitype(0x120);

    detector_init();

    // ID is already resolved; detector_init_late should immediately return 1
    int res = detector_init_late();
    ASSERT_EQ(res, 1);
}

static void test_detector_init_late_umd_success(void) {
    mock_pspsdk_reset();
    mock_cfw_clear_init_params();

    // Initial fetch produces UNKNOWN-00000
    mock_cfw_set_apitype(0x120);
    mock_cfw_set_executable_path("");
    detector_init();

    const GameMetadata *meta1 = detector_get_metadata();
    ASSERT_STR_EQ(meta1->game_id, "UNKNOWN-00000");

    // Configure UMD for late fetch
    mock_cfw_set_umd_file("ms0:/ISO/GranTurismo.iso");
    const char *umd_data = "UCUS-98732|1.00\r\n";
    mock_vfs_map_file("disc0:/UMD_DATA.BIN", umd_data, strlen(umd_data));

    int res = detector_init_late();
    ASSERT_EQ(res, 1);

    const GameMetadata *meta2 = detector_get_metadata();
    ASSERT_STR_EQ(meta2->game_id, "UCUS98732");
}

static void test_detector_init_late_umd_failure(void) {
    mock_pspsdk_reset();
    mock_cfw_clear_init_params();

    // Initial fetch produces UNKNOWN-00000
    mock_cfw_set_apitype(0x120);
    mock_cfw_set_executable_path("");
    detector_init();

    const GameMetadata *meta1 = detector_get_metadata();
    ASSERT_STR_EQ(meta1->game_id, "UNKNOWN-00000");

    // No UMD mounted
    mock_cfw_set_umd_file("");

    int res = detector_init_late();
    ASSERT_EQ(res, 0);

    const GameMetadata *meta2 = detector_get_metadata();
    ASSERT_STR_EQ(meta2->game_id, "UNKNOWN-00000");
}

void run_detector_tests(void) {
    TEST_RUN(test_detector_init_with_system_control);
    TEST_RUN(test_detector_init_late_when_already_known);
    TEST_RUN(test_detector_init_late_umd_success);
    TEST_RUN(test_detector_init_late_umd_failure);
}
