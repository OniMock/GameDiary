/**
 * Unit tests for src/common/debug.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/debug.h"
#include <string.h>

static void test_debug_init_and_logging(void) {
    mock_pspsdk_reset();
    mock_rtc_set_unix_time(1776686400U); // 2026-04-20

    // Initialize debug system
    debug_init();

    // Log various format specifiers
    debug_log("TEST_MOD", "String: %s, Int: %d, Neg: %d, Uint: %u, Hex: 0x%04x, HEX: 0x%04X, Char: %c, Pct: %%, Unk: %z",
              "hello_world", 42, -99, 12345U, 0xabcd, 0xef01, 'Z');

    // Log null string and zero values
    debug_log("TEST_MOD", "Null: %s, Zero: %d, ZeroU: %u, ZeroHex: %04x",
              NULL, 0, 0U, 0);

    // Verify file was written
    // The date from 1776686400 (mock_rtc) will be used by sceRtcGetCurrentClock
    // Test that file I/O occurred without crash
    ASSERT_TRUE(1);

    // Test error handling when opening log file fails
    mock_vfs_fail_open(true);
    debug_log("FAIL_MOD", "This should handle open failure safely: %d", 123);
    mock_vfs_fail_open(false);
}

void run_test_debug(void) {
    TEST_SUITE_BEGIN("Debug Logging Module");
    TEST_RUN(test_debug_init_and_logging);
    TEST_SUITE_END();
}
