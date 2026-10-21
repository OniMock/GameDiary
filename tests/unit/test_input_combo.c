/**
 * Unit tests for src/plugin/input_combo.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/input_combo.h"

#define COMBO_BUTTONS (PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT)

static void test_combo_disabled(void) {
    mock_pspsdk_reset();
    mock_ctrl_set_buttons(COMBO_BUTTONS);

    // When hotkey_enabled is 0, should never fire even if held indefinitely
    ASSERT_EQ(input_combo_poll(1000, 1, 0), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(5000, 1, 0), INPUT_COMBO_NONE);
}

static void test_combo_timing_and_debounce(void) {
    mock_pspsdk_reset();

    // No buttons pressed
    mock_ctrl_set_buttons(0);
    ASSERT_EQ(input_combo_poll(1000, 1, 1), INPUT_COMBO_NONE);

    // Press only L + R (missing Select)
    mock_ctrl_set_buttons(PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER);
    ASSERT_EQ(input_combo_poll(1100, 1, 1), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(4000, 1, 1), INPUT_COMBO_NONE);

    // Now press full combo at t=5000
    mock_ctrl_set_buttons(COMBO_BUTTONS);
    ASSERT_EQ(input_combo_poll(5000, 1, 1), INPUT_COMBO_NONE);

    // Advance 1000ms (t=6000, total held 1000ms < 2000ms)
    ASSERT_EQ(input_combo_poll(6000, 1, 1), INPUT_COMBO_NONE);

    // Advance to 1999ms held (t=6999)
    ASSERT_EQ(input_combo_poll(6999, 1, 1), INPUT_COMBO_NONE);

    // Advance to 2000ms held (t=7000) -> should FIRE TOGGLE!
    ASSERT_EQ(input_combo_poll(7000, 1, 1), INPUT_COMBO_TOGGLE);

    // Continue holding -> must NOT re-fire
    ASSERT_EQ(input_combo_poll(7100, 1, 1), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(10000, 1, 1), INPUT_COMBO_NONE);

    // Release buttons at t=10100
    mock_ctrl_set_buttons(0);
    ASSERT_EQ(input_combo_poll(10100, 1, 1), INPUT_COMBO_NONE);

    // Press again at t=11000 -> starts new hold cycle
    mock_ctrl_set_buttons(COMBO_BUTTONS);
    ASSERT_EQ(input_combo_poll(11000, 1, 1), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(12500, 1, 1), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(13000, 1, 1), INPUT_COMBO_TOGGLE);
}

static void test_combo_metadata_not_ready(void) {
    mock_pspsdk_reset();
    mock_ctrl_set_buttons(COMBO_BUTTONS);

    // Hold for 2000ms with metadata_ready = 0
    ASSERT_EQ(input_combo_poll(1000, 0, 1), INPUT_COMBO_NONE);
    ASSERT_EQ(input_combo_poll(3000, 0, 1), INPUT_COMBO_NOT_READY);

    // Continue holding
    ASSERT_EQ(input_combo_poll(3500, 0, 1), INPUT_COMBO_NONE);
}

void run_test_input_combo(void) {
    TEST_SUITE_BEGIN("Input Combo Detection Module");
    TEST_RUN(test_combo_disabled);
    TEST_RUN(test_combo_timing_and_debounce);
    TEST_RUN(test_combo_metadata_not_ready);
    TEST_SUITE_END();
}
