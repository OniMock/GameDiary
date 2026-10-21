/**
 * Unit tests for src/common/sfo_parser.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/sfo_parser.h"

/** Helper to generate a minimal valid .SFO binary buffer */
static size_t build_mock_sfo(u8 *buf, size_t max_buf, const char *title, const char *disc_id) {
    memset(buf, 0, max_buf);

    // SFO Header: 20 bytes
    // [0..3]: '\0PSF'
    buf[0] = 0x00; buf[1] = 'P'; buf[2] = 'S'; buf[3] = 'F';
    // [4..7]: version 1.1
    buf[4] = 0x01; buf[5] = 0x01; buf[6] = 0x00; buf[7] = 0x00;

    u32 num_entries = 2;
    u32 entries_size = num_entries * 16;
    u32 key_ofs = 20 + entries_size;

    // Keys: "TITLE\0" (6) + "DISC_ID\0" (8) = 14 bytes
    const char *k0 = "TITLE";
    const char *k1 = "DISC_ID";
    size_t k0_len = strlen(k0) + 1;
    size_t k1_len = strlen(k1) + 1;
    u32 data_ofs = key_ofs + (u32)(k0_len + k1_len);

    // Pad data_ofs to 4-byte boundary
    if (data_ofs % 4 != 0) data_ofs += (4 - (data_ofs % 4));

    // Fill header offsets
    memcpy(&buf[8], &key_ofs, 4);
    memcpy(&buf[12], &data_ofs, 4);
    memcpy(&buf[16], &num_entries, 4);

    // Entry 0: TITLE
    u16 k0_off = 0;
    u32 d0_len = (u32)strlen(title) + 1;
    u32 d0_off = 0;
    memcpy(&buf[20 + 0], &k0_off, 2);
    buf[20 + 2] = 0x04; buf[20 + 3] = 0x02; // UTF-8 string format
    memcpy(&buf[20 + 4], &d0_len, 4);
    memcpy(&buf[20 + 8], &d0_len, 4);
    memcpy(&buf[20 + 12], &d0_off, 4);

    // Entry 1: DISC_ID
    u16 k1_off = (u16)k0_len;
    u32 d1_len = (u32)strlen(disc_id) + 1;
    u32 d1_off = d0_len;
    memcpy(&buf[20 + 16 + 0], &k1_off, 2);
    buf[20 + 16 + 2] = 0x04; buf[20 + 16 + 3] = 0x02;
    memcpy(&buf[20 + 16 + 4], &d1_len, 4);
    memcpy(&buf[20 + 16 + 8], &d1_len, 4);
    memcpy(&buf[20 + 16 + 12], &d1_off, 4);

    // Write keys
    memcpy(&buf[key_ofs + k0_off], k0, k0_len);
    memcpy(&buf[key_ofs + k1_off], k1, k1_len);

    // Write data
    memcpy(&buf[data_ofs + d0_off], title, d0_len);
    memcpy(&buf[data_ofs + d1_off], disc_id, d1_len);

    return data_ofs + d1_off + d1_len;
}

static void test_sfo_read_string_success(void) {
    u8 sfo_buf[512];
    size_t sfo_size = build_mock_sfo(sfo_buf, sizeof(sfo_buf), "Castlevania: SotN", "SLUS00067");
    mock_vfs_map_file("ms0:/PARAM.SFO", sfo_buf, sfo_size);

    char out[64] = {0};
    int res = sfo_read_string("ms0:/PARAM.SFO", "TITLE", out, sizeof(out));
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(out, "Castlevania: SotN");

    memset(out, 0, sizeof(out));
    res = sfo_read_string("ms0:/PARAM.SFO", "DISC_ID", out, sizeof(out));
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(out, "SLUS00067");

    // Missing key
    memset(out, 0, sizeof(out));
    res = sfo_read_string("ms0:/PARAM.SFO", "NON_EXISTENT", out, sizeof(out));
    ASSERT_EQ(res, 0);
}

static void test_sfo_truncated_file(void) {
    u8 small[10] = { 0x00, 'P', 'S', 'F' };
    mock_vfs_map_file("ms0:/TRUNC.SFO", small, sizeof(small));

    char out[64] = {0};
    int res = sfo_read_string("ms0:/TRUNC.SFO", "TITLE", out, sizeof(out));
    ASSERT_EQ(res, 0);
}

static void test_sfo_invalid_magic(void) {
    u8 bad[32] = { 'B', 'A', 'D', '!' };
    mock_vfs_map_file("ms0:/BAD.SFO", bad, sizeof(bad));

    char out[64] = {0};
    int res = sfo_read_string("ms0:/BAD.SFO", "TITLE", out, sizeof(out));
    ASSERT_EQ(res, 0);
}

static void test_pbp_read_sfo_string(void) {
    u8 sfo_buf[512];
    size_t sfo_size = build_mock_sfo(sfo_buf, sizeof(sfo_buf), "Final Fantasy VII", "SCUS94163");

    u8 pbp[1024] = {0};
    pbp[0] = 0x00; pbp[1] = 'P'; pbp[2] = 'B'; pbp[3] = 'P';
    pbp[4] = 0x00; pbp[5] = 0x00; pbp[6] = 0x01; pbp[7] = 0x00;

    u32 sfo_offset = 64;
    memcpy(&pbp[8], &sfo_offset, 4);
    memcpy(&pbp[sfo_offset], sfo_buf, sfo_size);

    mock_vfs_map_file("ms0:/PSP/GAME/FF7/EBOOT.PBP", pbp, sizeof(pbp));

    char out[64] = {0};
    int res = pbp_read_sfo_string("ms0:/PSP/GAME/FF7/EBOOT.PBP", "TITLE", out, sizeof(out));
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(out, "Final Fantasy VII");

    memset(out, 0, sizeof(out));
    res = pbp_read_sfo_string("ms0:/PSP/GAME/FF7/EBOOT.PBP", "DISC_ID", out, sizeof(out));
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(out, "SCUS94163");

    // Invalid PBP header
    u8 bad_pbp[64] = {0};
    mock_vfs_map_file("ms0:/BAD.PBP", bad_pbp, sizeof(bad_pbp));
    res = pbp_read_sfo_string("ms0:/BAD.PBP", "TITLE", out, sizeof(out));
    ASSERT_EQ(res, 0);

    // Non-existent PBP file
    ASSERT_EQ(pbp_read_sfo_string("ms0:/NON_EXISTENT.PBP", "TITLE", out, sizeof(out)), 0);

    // Truncated PBP (< 12 bytes)
    u8 tiny_pbp[6] = { 0x00, 'P', 'B', 'P' };
    mock_vfs_map_file("ms0:/TINY.PBP", tiny_pbp, sizeof(tiny_pbp));
    ASSERT_EQ(pbp_read_sfo_string("ms0:/TINY.PBP", "TITLE", out, sizeof(out)), 0);
}

static void test_sfo_non_existent_and_buffer_truncation(void) {
    char out[10];

    // Non-existent SFO
    ASSERT_EQ(sfo_read_string("ms0:/DOES_NOT_EXIST.SFO", "TITLE", out, sizeof(out)), 0);

    // Buffer smaller than title (truncation test)
    u8 sfo_buf[512];
    size_t sfo_size = build_mock_sfo(sfo_buf, sizeof(sfo_buf), "A Very Long Game Title Here", "ABCD12345");
    mock_vfs_map_file("ms0:/LONG_TITLE.SFO", sfo_buf, sfo_size);

    char short_buf[6] = {0};
    int res = sfo_read_string("ms0:/LONG_TITLE.SFO", "TITLE", short_buf, sizeof(short_buf));
    ASSERT_EQ(res, 1);
    ASSERT_EQ(strlen(short_buf), 5); // 5 chars + null terminator
}

void run_test_sfo_parser(void) {
    TEST_SUITE_BEGIN("SFO Parser Module");
    mock_pspsdk_reset();
    TEST_RUN(test_sfo_read_string_success);
    TEST_RUN(test_sfo_truncated_file);
    TEST_RUN(test_sfo_invalid_magic);
    TEST_RUN(test_pbp_read_sfo_string);
    TEST_RUN(test_sfo_non_existent_and_buffer_truncation);
    TEST_SUITE_END();
}
