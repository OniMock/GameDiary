/**
 * Unit tests for src/plugin/metadata_repository.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/metadata_repository.h"
#include "common/models.h"
#include "common/utils.h"
#include <string.h>

static void test_metadata_is_psp_app(void) {
    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));

    // NULL metadata check
    ASSERT_EQ(metadata_is_psp_app(NULL), 0);

    // 1. apitype 0x143 is MS_APP
    strncpy(meta.apitype_str, "0x143", sizeof(meta.apitype_str));
    ASSERT_EQ(metadata_is_psp_app(&meta), 1);

    // 2. Official PSP/APP directory path
    strncpy(meta.apitype_str, "0x141", sizeof(meta.apitype_str));
    strncpy(meta.file_path, "ms0:/PSP/APP/NPIA00013/EBOOT.PBP", sizeof(meta.file_path));
    ASSERT_EQ(metadata_is_psp_app(&meta), 1);

    // 3. Sony Title ID folder (NPIA00013) inside PSP/GAME
    strncpy(meta.file_path, "ms0:/PSP/GAME/NPIA00013/EBOOT.PBP", sizeof(meta.file_path));
    ASSERT_EQ(metadata_is_psp_app(&meta), 1);

    // 4. Regular homebrew folder
    strncpy(meta.file_path, "ms0:/PSP/GAME/Doom-PSP/EBOOT.PBP", sizeof(meta.file_path));
    ASSERT_EQ(metadata_is_psp_app(&meta), 0);

    // 5. Empty path
    meta.file_path[0] = '\0';
    ASSERT_EQ(metadata_is_psp_app(&meta), 0);
}

static void test_homebrew_fallback_id_normalization(void) {
    const char *names[] = {
        "gamediary",
        "GameDiary",
        "Game Diary",
        "GAME DIARY",
        "   G a m e D i a r y   "
    };

    for (size_t i = 0; i < sizeof(names)/sizeof(names[0]); i++) {
        char normalized[64];
        int out = 0;
        for (int j = 0; names[i][j] != '\0' && out < (int)sizeof(normalized) - 1; j++) {
            char c = names[i][j];
            if (c == ' ') continue;
            if (c >= 'A' && c <= 'Z') c = c + ('a' - 'A');
            normalized[out++] = c;
        }
        normalized[out] = '\0';

        char generated_id[16];
        snprintf(generated_id, sizeof(generated_id), "HBX%08X", (unsigned int)hash_string(normalized));
        ASSERT_STR_EQ(generated_id, "HBX0E9162F8");
    }
}

static void test_metadata_fetch_from_umd_skipped_non_psp(void) {
    mock_pspsdk_reset();
    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));
    meta.category = CAT_HOMEBREW;
    ASSERT_EQ(metadata_fetch_from_umd(&meta), 0);
    ASSERT_EQ(metadata_fetch_from_umd(NULL), 0);
}

static void test_metadata_fetch_from_umd_success(void) {
    mock_pspsdk_reset();
    mock_cfw_set_umd_file("ms0:/ISO/GranTurismo.iso");

    // Write UMD_DATA.BIN
    const char *umd_data = "UCUS-98732|1.00\r\n";
    mock_vfs_map_file("disc0:/UMD_DATA.BIN", umd_data, strlen(umd_data));

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));
    meta.category = CAT_PSP;

    int ret = metadata_fetch_from_umd(&meta);
    ASSERT_EQ(ret, 1);
    ASSERT_STR_EQ(meta.game_id, "UCUS98732");
    ASSERT_EQ(meta.category, CAT_PSP);
}

static void test_metadata_fetch_psp_game(void) {
    mock_pspsdk_reset();
    mock_cfw_set_init_param("DISC_ID", "ULUS10041", 9);
    mock_cfw_set_init_param("TITLE", "Grand Theft Auto", 16);
    mock_cfw_set_apitype(0x120);

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));

    int res = metadata_fetch(&meta);
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(meta.game_id, "ULUS10041");
    ASSERT_STR_EQ(meta.game_name, "Grand Theft Auto");
    ASSERT_EQ(meta.category, CAT_PSP);
}

static void test_metadata_fetch_homebrew_spoofed_id_rejected(void) {
    mock_pspsdk_reset();
    // CFW spoofed DISC_ID (LocoRoco)
    mock_cfw_set_init_param("DISC_ID", "UCJS10041", 9);
    mock_cfw_set_init_param("TITLE", "Doom PSP", 8);
    mock_cfw_set_apitype(0x141); // Homebrew
    mock_cfw_set_executable_path("ms0:/PSP/GAME/Doom-PSP/EBOOT.PBP");

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));

    int res = metadata_fetch(&meta);
    ASSERT_EQ(res, 1);
    ASSERT_EQ(meta.category, CAT_HOMEBREW);
    // Spoofed UCJS10041 must be rejected in favor of HBX hash or folder
    ASSERT_FALSE(strcmp(meta.game_id, "UCJS10041") == 0);
    ASSERT_TRUE(strncmp(meta.game_id, "HBX", 3) == 0);
}

static void test_metadata_fetch_from_umd_drive_stat_ready(void) {
    mock_pspsdk_reset();
    mock_cfw_set_umd_file(""); // Physical UMD
    mock_cfw_set_umd_stat(0x20); // PSP_UMD_READY_FLAG

    const char *umd_data = "ULES-00123|1.00\r\n";
    mock_vfs_map_file("disc0:/UMD_DATA.BIN", umd_data, strlen(umd_data));

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));
    meta.category = CAT_PSP;

    int ret = metadata_fetch_from_umd(&meta);
    ASSERT_EQ(ret, 1);
    ASSERT_STR_EQ(meta.game_id, "ULES00123");
}

static void test_metadata_fetch_from_umd_drive_stat_not_ready(void) {
    mock_pspsdk_reset();
    mock_cfw_set_umd_file(""); // Physical UMD
    mock_cfw_set_umd_stat(0x00); // NOT ready

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));
    meta.category = CAT_PSP;

    int ret = metadata_fetch_from_umd(&meta);
    ASSERT_EQ(ret, 0); // Must be skipped
}

static size_t build_test_sfo(u8 *buf, size_t max_buf, const char *title, const char *disc_id) {
    memset(buf, 0, max_buf);
    buf[0] = 0x00; buf[1] = 'P'; buf[2] = 'S'; buf[3] = 'F';
    buf[4] = 0x01; buf[5] = 0x01; buf[6] = 0x00; buf[7] = 0x00;
    u32 num_entries = 2;
    u32 entries_size = num_entries * 16;
    u32 key_ofs = 20 + entries_size;
    const char *k0 = "TITLE";
    const char *k1 = "DISC_ID";
    size_t k0_len = strlen(k0) + 1;
    size_t k1_len = strlen(k1) + 1;
    u32 data_ofs = key_ofs + (u32)(k0_len + k1_len);
    if (data_ofs % 4 != 0) data_ofs += (4 - (data_ofs % 4));
    memcpy(&buf[8], &key_ofs, 4);
    memcpy(&buf[12], &data_ofs, 4);
    memcpy(&buf[16], &num_entries, 4);
    u16 k0_off = 0;
    u32 d0_len = (u32)strlen(title) + 1;
    u32 d0_off = 0;
    memcpy(&buf[20 + 0], &k0_off, 2);
    buf[20 + 2] = 0x04; buf[20 + 3] = 0x02;
    memcpy(&buf[20 + 4], &d0_len, 4);
    memcpy(&buf[20 + 8], &d0_len, 4);
    memcpy(&buf[20 + 12], &d0_off, 4);
    u16 k1_off = (u16)k0_len;
    u32 d1_len = (u32)strlen(disc_id) + 1;
    u32 d1_off = d0_len;
    if (d1_off % 4 != 0) d1_off += (4 - (d1_off % 4));
    memcpy(&buf[36 + 0], &k1_off, 2);
    buf[36 + 2] = 0x04; buf[36 + 3] = 0x02;
    memcpy(&buf[36 + 4], &d1_len, 4);
    memcpy(&buf[36 + 8], &d1_len, 4);
    memcpy(&buf[36 + 12], &d1_off, 4);
    memcpy(&buf[key_ofs], k0, k0_len);
    memcpy(&buf[key_ofs + k0_len], k1, k1_len);
    memcpy(&buf[data_ofs + d0_off], title, d0_len);
    memcpy(&buf[data_ofs + d1_off], disc_id, d1_len);
    return data_ofs + d1_off + d1_len;
}

static void test_metadata_fetch_ps1_fallbacks(void) {
    mock_pspsdk_reset();
    mock_cfw_set_apitype(0x144); // PS1 apitype
    mock_cfw_set_executable_path("ms0:/PSP/GAME/Castlevania/EBOOT.PBP");

    // 1. EBOOT with valid DISC_ID and empty TITLE -> fallback "PS1: %s"
    u8 sfo_buf[512];
    size_t sfo_size = build_test_sfo(sfo_buf, sizeof(sfo_buf), "", "SLUS00067");

    u8 pbp[1024] = {0};
    pbp[0] = 0x00; pbp[1] = 'P'; pbp[2] = 'B'; pbp[3] = 'P';
    pbp[4] = 0x00; pbp[5] = 0x00; pbp[6] = 0x01; pbp[7] = 0x00;
    u32 sfo_offset = 64;
    memcpy(&pbp[8], &sfo_offset, 4);
    memcpy(&pbp[sfo_offset], sfo_buf, sfo_size);

    mock_vfs_map_file("ms0:/PSP/GAME/Castlevania/EBOOT.PBP", pbp, sizeof(pbp));

    GameMetadata meta;
    memset(&meta, 0, sizeof(meta));

    int res = metadata_fetch(&meta);
    ASSERT_EQ(res, 1);
    ASSERT_EQ(meta.category, CAT_PS1);
    ASSERT_STR_EQ(meta.game_name, "PS1: SLUS00067");

    // 2. EBOOT with empty DISC_ID and empty TITLE -> fallback "PT: %s"
    sfo_size = build_test_sfo(sfo_buf, sizeof(sfo_buf), "", "");
    memcpy(&pbp[sfo_offset], sfo_buf, sfo_size);
    mock_vfs_map_file("ms0:/PSP/GAME/Castlevania/EBOOT.PBP", pbp, sizeof(pbp));

    memset(&meta, 0, sizeof(meta));
    res = metadata_fetch(&meta);
    ASSERT_EQ(res, 1);
    ASSERT_STR_EQ(meta.game_name, "PT: EBOOT.PBP");
}

void run_test_metadata_repository(void) {
    TEST_SUITE_BEGIN("Metadata Repository Module");
    TEST_RUN(test_metadata_is_psp_app);
    TEST_RUN(test_homebrew_fallback_id_normalization);
    TEST_RUN(test_metadata_fetch_from_umd_skipped_non_psp);
    TEST_RUN(test_metadata_fetch_from_umd_success);
    TEST_RUN(test_metadata_fetch_psp_game);
    TEST_RUN(test_metadata_fetch_homebrew_spoofed_id_rejected);
    TEST_RUN(test_metadata_fetch_from_umd_drive_stat_ready);
    TEST_RUN(test_metadata_fetch_from_umd_drive_stat_not_ready);
    TEST_RUN(test_metadata_fetch_ps1_fallbacks);
    TEST_SUITE_END();
}
