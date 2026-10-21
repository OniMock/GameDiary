/**
 * Unit tests for src/common/psp_hardware.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/psp_hardware.h"

static void test_psp_hardware_low_end_detection(void) {
    mock_pspsdk_reset();
    psp_hardware_reset_cache();

    // 1. Emulate PSP-1000 with 24MB free RAM (< 34MB)
    mock_hardware_set_total_free_mem(24 * 1024 * 1024);

    ASSERT_EQ(is_psp_low_end(), 1);
    ASSERT_EQ(is_psp_1000(), 1);
    ASSERT_EQ(psp_get_model(), 0);

    // 2. Verify result is cached even if memory changes
    mock_hardware_set_total_free_mem(50 * 1024 * 1024);
    ASSERT_EQ(is_psp_low_end(), 1);
    ASSERT_EQ(is_psp_1000(), 1);
    ASSERT_EQ(psp_get_model(), 0);

    // 3. Reset cache and test high-end PSP (Slim/Brite/Go/Street with 50MB free RAM)
    psp_hardware_reset_cache();
    ASSERT_EQ(is_psp_low_end(), 0);
    ASSERT_EQ(is_psp_1000(), 0);
    ASSERT_EQ(psp_get_model(), 1);

    // 4. Verify result is cached
    mock_hardware_set_total_free_mem(10 * 1024 * 1024);
    ASSERT_EQ(is_psp_low_end(), 0);
    ASSERT_EQ(is_psp_1000(), 0);
    ASSERT_EQ(psp_get_model(), 1);

    psp_hardware_reset_cache();
}

void run_test_psp_hardware(void) {
    TEST_SUITE_BEGIN("PSP Hardware Module");
    TEST_RUN(test_psp_hardware_low_end_detection);
    TEST_SUITE_END();
}
