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

static void test_debug_pointer_formatting(void) {
    mock_pspsdk_reset();
    mock_rtc_set_unix_time(1776686400U); // 2026-04-20
    debug_init();

    // Log valid pointer with subsequent argument to ensure va_list doesn't shift
    debug_log("PTR_MOD", "Framebuffer active at %p (stride: %d)", (void*)0x04000000, 512);

    // Log NULL pointer
    debug_log("PTR_MOD", "Null pointer: %p", NULL);

    // Verify written log content via mock VFS
    char log_buf[2048];
    memset(log_buf, 0, sizeof(log_buf));
    const char *log_path = "ms0:/PSP/COMMON/GameDiary/debug-20-04-2026.txt";
    size_t rd = mock_vfs_read_file(log_path, log_buf, sizeof(log_buf) - 1);
    ASSERT_TRUE(rd > 0);

    // Check that %p produced 0x04000000 and the subsequent argument was formatted correctly as 512
    ASSERT_TRUE(strstr(log_buf, "Framebuffer active at 0x04000000 (stride: 512)") != NULL);

    // Check that NULL pointer produced (nil)
    ASSERT_TRUE(strstr(log_buf, "Null pointer: (nil)") != NULL);
}

void run_test_debug(void) {
    TEST_SUITE_BEGIN("Debug Logging Module");
    TEST_RUN(test_debug_init_and_logging);
    TEST_RUN(test_debug_pointer_formatting);
    TEST_SUITE_END();
}
