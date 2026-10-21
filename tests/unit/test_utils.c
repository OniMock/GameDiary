/**
 * Unit tests for src/common/utils.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/utils.h"
#include "common/db_schema.h"
#include <string.h>

static void test_hash_string(void) {
    ASSERT_EQ(hash_string(""), 5381U);
    u32 hash = hash_string("gamediary");
    ASSERT_EQ(hash, 0x0E9162F8);
    ASSERT_NE(hash_string("game"), hash_string("Game"));
    ASSERT_NE(hash_string("ULUS10041"), hash_string("ULES00001"));
}

static void test_get_u32_le(void) {
    u8 bytes[4] = { 0x78, 0x56, 0x34, 0x12 };
    ASSERT_EQ(utils_get_u32_le(bytes), 0x12345678U);

    u8 zero[4] = { 0, 0, 0, 0 };
    ASSERT_EQ(utils_get_u32_le(zero), 0U);

    u8 max[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    ASSERT_EQ(utils_get_u32_le(max), 0xFFFFFFFFU);
}

static void test_time_overlap(void) {
    // Session entirely before window
    ASSERT_EQ(utils_time_overlap_secs(100, 200, 300, 400), 0U);

    // Session entirely after window
    ASSERT_EQ(utils_time_overlap_secs(500, 600, 300, 400), 0U);

    // Session spans entire window
    ASSERT_EQ(utils_time_overlap_secs(100, 500, 200, 400), 200U);

    // Session overlaps window start
    ASSERT_EQ(utils_time_overlap_secs(100, 300, 200, 400), 100U);

    // Session overlaps window end
    ASSERT_EQ(utils_time_overlap_secs(300, 500, 200, 400), 100U);

    // Session completely inside window
    ASSERT_EQ(utils_time_overlap_secs(100, 300, 150, 250), 100U);

    // Exact match
    ASSERT_EQ(utils_time_overlap_secs(100, 200, 100, 200), 100U);

    // Inverted/empty interval (start >= end)
    ASSERT_EQ(utils_time_overlap_secs(200, 100, 50, 300), 0U);
    ASSERT_EQ(utils_time_overlap_secs(100, 100, 50, 300), 0U);
}

static void test_timestamp_and_timezone(void) {
    mock_rtc_set_unix_time(1776772800U);
    ASSERT_EQ(utils_get_timestamp(), 1776772800U);

    mock_rtc_set_timezone_offset(-10800); // UTC-3
    ASSERT_EQ(utils_get_timezone_offset_seconds(), -10800);

    mock_rtc_set_timezone_offset(3600); // UTC+1
    ASSERT_EQ(utils_get_timezone_offset_seconds(), 3600);

    u32 ms = utils_get_time_ms();
    ASSERT_TRUE(ms > 0);
}

static void test_copy_file(void) {
    const char *src = "ms0:/test_src.txt";
    const char *dst = "ms0:/test_dst.txt";
    const char *content = "Hello GameDiary Copy Test!";

    mock_vfs_map_file(src, content, strlen(content));
    int res = utils_copy_file(src, dst);
    ASSERT_EQ(res, 0);

    char read_buf[64] = {0};
    size_t rd = mock_vfs_read_file(dst, read_buf, sizeof(read_buf) - 1);
    ASSERT_EQ(rd, strlen(content));
    ASSERT_STR_EQ(read_buf, content);

    // Copy missing source
    res = utils_copy_file("ms0:/missing.txt", "ms0:/out.txt");
    ASSERT_EQ(res, -1);
}

static void test_extract_pbp_icon(void) {
    u8 pbp[300] = {0};
    pbp[0] = 0x00; pbp[1] = 'P'; pbp[2] = 'B'; pbp[3] = 'P';
    pbp[4] = 0x00; pbp[5] = 0x00; pbp[6] = 0x01; pbp[7] = 0x00; // 0x00010000

    // Offsets little endian
    pbp[8]  = 40;  // param.sfo
    pbp[12] = 100; // icon0 offset = 100
    pbp[16] = 200; // next section offset = 200 (icon size = 100 bytes)

    // Put mock icon data at offset 100
    memset(&pbp[100], 0xAA, 100);

    mock_vfs_map_file("ms0:/GAME/TEST/EBOOT.PBP", pbp, sizeof(pbp));

    int res = utils_extract_pbp_icon("ms0:/GAME/TEST/EBOOT.PBP", "ms0:/extracted_icon.png");
    ASSERT_EQ(res, 0);

    u8 extracted[150] = {0};
    size_t rd = mock_vfs_read_file("ms0:/extracted_icon.png", extracted, sizeof(extracted));
    ASSERT_EQ(rd, 100U);
    ASSERT_EQ(extracted[0], 0xAA);
    ASSERT_EQ(extracted[99], 0xAA);

    // Invalid PBP magic
    u8 bad_pbp[100] = {0};
    mock_vfs_map_file("ms0:/GAME/TEST/BAD.PBP", bad_pbp, sizeof(bad_pbp));
    res = utils_extract_pbp_icon("ms0:/GAME/TEST/BAD.PBP", "ms0:/bad.png");
    ASSERT_EQ(res, -3);
}

static void test_extract_pbp_edge_cases(void) {
    mock_pspsdk_reset();

    // 1. Missing file
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/missing.pbp", "ms0:/icon.png"), -1);

    // 2. Truncated header (< 40 bytes)
    u8 trunc[20] = {0};
    mock_vfs_map_file("ms0:/trunc.pbp", trunc, sizeof(trunc));
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/trunc.pbp", "ms0:/icon.png"), -2);

    // 3. Icon offset is zero
    u8 zero_icon[64] = { 0x00, 'P', 'B', 'P', 0x00, 0x00, 0x01, 0x00 };
    mock_vfs_map_file("ms0:/zero_icon.pbp", zero_icon, sizeof(zero_icon));
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/zero_icon.pbp", "ms0:/icon.png"), -4);

    // 4. Next offset is zero (no section after icon)
    u8 no_next[64] = { 0x00, 'P', 'B', 'P', 0x00, 0x00, 0x01, 0x00 };
    no_next[12] = 40; // icon0 offset = 40
    mock_vfs_map_file("ms0:/no_next.pbp", no_next, sizeof(no_next));
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/no_next.pbp", "ms0:/icon.png"), -5);

    // 5. Size exceeds 1MB limit (> 1024*1024)
    u8 huge_icon_pbp[64] = { 0x00, 'P', 'B', 'P', 0x00, 0x00, 0x01, 0x00 };
    u32 ofs1 = 40;
    u32 ofs2 = 40 + (2 * 1024 * 1024); // 2MB
    memcpy(&huge_icon_pbp[12], &ofs1, 4);
    memcpy(&huge_icon_pbp[16], &ofs2, 4);
    mock_vfs_map_file("ms0:/huge_icon.pbp", huge_icon_pbp, sizeof(huge_icon_pbp));
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/huge_icon.pbp", "ms0:/icon.png"), -6);

    // 6. Destination open failure
    u8 valid_pbp[128] = { 0x00, 'P', 'B', 'P', 0x00, 0x00, 0x01, 0x00 };
    u32 valid_ofs1 = 40;
    u32 valid_ofs2 = 60;
    memcpy(&valid_pbp[12], &valid_ofs1, 4);
    memcpy(&valid_pbp[16], &valid_ofs2, 4);
    mock_vfs_map_file("ms0:/valid.pbp", valid_pbp, sizeof(valid_pbp));
    mock_vfs_fail_path("unwritable.png");
    ASSERT_EQ(utils_extract_pbp_icon("ms0:/valid.pbp", "ms0:/unwritable.png"), -7);

    // Copy file failure on destination open
    mock_vfs_map_file("ms0:/source.bin", "data", 4);
    ASSERT_EQ(utils_copy_file("ms0:/source.bin", "ms0:/unwritable.png"), -2);
    mock_vfs_fail_path(NULL);
}

static void test_storage_dirs_and_prefixes(void) {
    mock_pspsdk_reset();

    const char *prefix = utils_get_device_prefix();
    ASSERT_NOT_NULL(prefix);

    utils_ensure_storage_dirs(prefix);
}

static void test_logging(void) {
    mock_rtc_set_unix_time(1700000000U);
    utils_set_log_context("ULES-00123");
    utils_log_error("TEST", "Sample error message", -42);
    utils_log_trace("TEST", "Sample trace message");

    char log_buf[1024] = {0};
    mock_vfs_read_file("ms0:" GDIARY_BASE_DIR "/error.txt", log_buf, sizeof(log_buf) - 1);
    ASSERT_TRUE(strstr(log_buf, "[ERR] [TEST] Sample error message (Code: -42)") != NULL);
    ASSERT_TRUE(strstr(log_buf, "[TRC] [TEST] Sample trace message") != NULL);
    ASSERT_TRUE(strstr(log_buf, "ULES-00123") != NULL);

    // Log failure when open fails
    mock_vfs_fail_open(true);
    utils_log_error("TEST", "Should not crash", -1);
    utils_log_trace("TEST", "Should not crash");
    mock_vfs_fail_open(false);
}

void run_test_utils(void) {
    TEST_SUITE_BEGIN("Utils Module");
    mock_pspsdk_reset();
    TEST_RUN(test_hash_string);
    TEST_RUN(test_get_u32_le);
    TEST_RUN(test_time_overlap);
    TEST_RUN(test_timestamp_and_timezone);
    TEST_RUN(test_copy_file);
    TEST_RUN(test_extract_pbp_icon);
    TEST_RUN(test_extract_pbp_edge_cases);
    TEST_RUN(test_storage_dirs_and_prefixes);
    TEST_RUN(test_logging);
    TEST_SUITE_END();
}
