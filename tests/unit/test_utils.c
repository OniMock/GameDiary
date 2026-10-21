/**
 * Unit tests for src/common/utils.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/utils.h"
#include "common/db_schema.h"

static void test_hash_string(void) {
    // DJB2 hash of empty string is 5381
    ASSERT_EQ(hash_string(""), 5381U);

    // Self-identification hash verification (GDIARY_SELF_ID = "HBX0E9162F8")
    // djb2("gamediary") should equal 0x0E9162F8
    u32 hash = hash_string("gamediary");
    ASSERT_EQ(hash, 0x0E9162F8U);

    // Different strings produce different hashes
    ASSERT_NE(hash_string("game"), hash_string("Game"));
    ASSERT_NE(hash_string("ULUS10041"), hash_string("ULES00001"));
}

static void test_get_u32_le(void) {
    u8 buf[8] = { 0x00, 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00 };
    // Read at unaligned offset 1
    u32 val = utils_get_u32_le(&buf[1]);
    ASSERT_EQ(val, 0x78563412U);

    u8 buf2[4] = { 0x47, 0x41, 0x4D, 0x44 }; // "GAMD"
    ASSERT_EQ(utils_get_u32_le(buf2), 0x444D4147U);
}

static void test_time_overlap(void) {
    // Disjoint intervals (no overlap)
    ASSERT_EQ(utils_time_overlap_secs(100, 200, 300, 400), 0U);
    ASSERT_EQ(utils_time_overlap_secs(300, 400, 100, 200), 0U);

    // Contiguous intervals (half-open, so no overlap at boundary)
    ASSERT_EQ(utils_time_overlap_secs(100, 200, 200, 300), 0U);

    // Partial overlap
    ASSERT_EQ(utils_time_overlap_secs(100, 250, 200, 300), 50U);
    ASSERT_EQ(utils_time_overlap_secs(200, 300, 100, 250), 50U);

    // Complete overlap (A inside B)
    ASSERT_EQ(utils_time_overlap_secs(150, 250, 100, 300), 100U);

    // Complete overlap (B inside A)
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
    // Construct a minimal valid PBP header
    // Offset 0: \0PBP (4 bytes)
    // Offset 4: version (4 bytes)
    // Offset 8: param.sfo offset (4 bytes) -> 40
    // Offset 12: icon0.png offset (4 bytes) -> 100
    // Offset 16: icon1.pmf offset (4 bytes) -> 200
    // Offset 20..36: other offsets
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
}

static void test_format_duration_compact_stub(void) {
    char buf[32];
    memset(buf, 0xAA, sizeof(buf));
    utils_format_duration_compact(3665, buf, sizeof(buf));
    /* BUG-02 Verification:
     * In src/common/utils.c:
     * void utils_format_duration_compact(u32 seconds, char *out, size_t size) {
     *   (void)seconds; (void)out; (void)size;
     * }
     * The function is an empty stub without implementation or null-termination.
     * We verify that the buffer was left untouched with canary bytes 0xAA. */
    ASSERT_EQ((unsigned char)buf[0], 0xAA);
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
    TEST_RUN(test_logging);
    TEST_RUN(test_format_duration_compact_stub);
    TEST_SUITE_END();
}
