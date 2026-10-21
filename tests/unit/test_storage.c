/**
 * Unit tests for src/common/storage.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "common/models.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static void test_storage_init_creates_files(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    // Verify directories created
    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary"));
    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/db"));
    ASSERT_TRUE(mock_vfs_file_exists("ms0:/PSP/COMMON/GameDiary/icons"));

    // Verify games.dat created with valid header
    char path[256];
    snprintf(path, sizeof(path), "%s/db/%s", k_base_dir, GAMES_DAT);
    ASSERT_TRUE(mock_vfs_file_exists(path));

    GameRegistryHeader hdr;
    size_t rd = mock_vfs_read_file(path, &hdr, sizeof(hdr));
    ASSERT_EQ(rd, sizeof(hdr));
    ASSERT_EQ(hdr.magic, GAMEDIARY_MAGIC);
    ASSERT_EQ(hdr.version, DB_VERSION);
    ASSERT_EQ(hdr.num_entries, 0U);
    ASSERT_EQ(hdr.next_uid, 1U);
    ASSERT_EQ(hdr.ready_flag, GAMEDIARY_MAGIC);
}

static void test_storage_get_or_create_game(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    GameMetadata meta1 = {
        .game_id = "ULUS-10041",
        .game_name = "Grand Theft Auto",
        .apitype_str = "0x120",
        .category = CAT_PSP,
        .file_path = "ms0:/ISO/GTA.ISO"
    };

    u32 uid1 = 0;
    int res = storage_get_or_create_game(&meta1, &uid1);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(uid1, 1U);

    // Calling again with same game_id returns SAME UID (from cache)
    u32 uid1_again = 0;
    res = storage_get_or_create_game(&meta1, &uid1_again);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(uid1_again, 1U);

    // Register second game
    GameMetadata meta2 = {
        .game_id = "SLUS-00067",
        .game_name = "Castlevania",
        .apitype_str = "0x144",
        .category = CAT_PS1,
        .file_path = "ms0:/PSP/GAME/SOTN/EBOOT.PBP"
    };

    u32 uid2 = 0;
    res = storage_get_or_create_game(&meta2, &uid2);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(uid2, 2U);

    // Safety: UNKNOWN-00000 for PSP must be rejected
    GameMetadata meta_unk = {
        .game_id = "UNKNOWN-00000",
        .category = CAT_PSP,
        .file_path = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN"
    };
    u32 uid_unk = 0;
    res = storage_get_or_create_game(&meta_unk, &uid_unk);
    ASSERT_EQ(res, -3); // Rejected for safety
}

static void test_storage_log_session(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    SceOff offset = -1;
    // Log initial 60s session (offset == -1 -> append)
    int res = storage_log_session(1, 60, 1700000000U, &offset);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(offset, 0); // First session starts at byte 0

    // Update the same session to 120s (offset >= 0 -> overwrite in place)
    res = storage_log_session(1, 120, 1700000000U, &offset);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(offset, 0);

    // Verify session data in file
    char path[256];
    snprintf(path, sizeof(path), "%s/db/%s", k_base_dir, SESSIONS_DAT);
    SessionEntry entry;
    size_t rd = mock_vfs_read_file(path, &entry, sizeof(entry));
    ASSERT_EQ(rd, sizeof(entry));
    ASSERT_EQ(entry.game_uid, 1U);
    ASSERT_EQ(entry.duration, 120U);
    ASSERT_EQ(entry.timestamp, 1700000000U);

    // Append second session
    SceOff offset2 = -1;
    res = storage_log_session(2, 300, 1700010000U, &offset2);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(offset2, (SceOff)sizeof(SessionEntry));
}

static void test_storage_session_repair_on_init(void) {
    mock_pspsdk_reset();

    // Create a sessions.dat with 1 full entry + 5 corrupted partial bytes
    char s_path[256];
    snprintf(s_path, sizeof(s_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

    u8 corrupted[sizeof(SessionEntry) + 5];
    memset(corrupted, 0x55, sizeof(corrupted));
    SessionEntry *e = (SessionEntry *)corrupted;
    e->game_uid = 42;
    e->duration = 1800;
    e->timestamp = 1700000000U;

    mock_vfs_map_file(s_path, corrupted, sizeof(corrupted));

    // Calling storage_init must detect non-multiple size and truncate partial entry
    storage_init(k_base_dir);

    u8 repaired[sizeof(corrupted)];
    size_t rd = mock_vfs_read_file(s_path, repaired, sizeof(repaired));
    ASSERT_EQ(rd, sizeof(SessionEntry)); // Exact size of 1 session entry!

    SessionEntry *repaired_e = (SessionEntry *)repaired;
    ASSERT_EQ(repaired_e->game_uid, 42U);
    ASSERT_EQ(repaired_e->duration, 1800U);
}

void run_test_storage(void) {
    TEST_SUITE_BEGIN("Storage & Persistence Module");
    TEST_RUN(test_storage_init_creates_files);
    TEST_RUN(test_storage_get_or_create_game);
    TEST_RUN(test_storage_log_session);
    TEST_RUN(test_storage_session_repair_on_init);
    TEST_SUITE_END();
}
