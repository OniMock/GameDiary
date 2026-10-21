/**
 * Unit tests for src/plugin/apitype.c
 */

#include "../framework/test_framework.h"
#include "plugin/apitype.h"

static void test_apitype_categories(void) {
    // VSH
    ASSERT_EQ(apitype_detect_category(0x200), CAT_VSH);
    ASSERT_EQ(apitype_detect_category(0x210), CAT_VSH);
    ASSERT_EQ(apitype_detect_category(0x300), CAT_VSH);

    // POPS / PS1
    ASSERT_EQ(apitype_detect_category(0x144), CAT_PS1);
    ASSERT_EQ(apitype_detect_category(0x155), CAT_PS1);
    ASSERT_EQ(apitype_detect_category(0x143), CAT_PS1);

    // PSP (UMD / ISO / PBP)
    ASSERT_EQ(apitype_detect_category(0x120), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x160), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x123), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x124), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x125), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x126), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x130), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x110), CAT_PSP);
    ASSERT_EQ(apitype_detect_category(0x115), CAT_PSP);

    // Homebrew
    ASSERT_EQ(apitype_detect_category(0x141), CAT_HOMEBREW);
    ASSERT_EQ(apitype_detect_category(0x152), CAT_HOMEBREW);

    // Unknown / Out of range
    ASSERT_EQ(apitype_detect_category(0x000), CAT_UNKNOWN);
    ASSERT_EQ(apitype_detect_category(0x100), CAT_UNKNOWN);
    ASSERT_EQ(apitype_detect_category(0x199), CAT_UNKNOWN);
    ASSERT_EQ(apitype_detect_category(-1), CAT_UNKNOWN);
}

void run_test_apitype(void) {
    TEST_SUITE_BEGIN("API Type Detection Module");
    TEST_RUN(test_apitype_categories);
    TEST_SUITE_END();
}
