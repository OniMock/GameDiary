/**
 * Integration Test: Storage Recovery & Crash Resilience
 * Tests recovery from interrupted atomic renames, header corruptions, and truncated session files.
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "common/models.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static u32 calc_checksum(const GameRegistryHeader *h) {
    u32 hash = 2166136261U;
    const unsigned char *p = (const unsigned char *)h;
    size_t skip_offset = offsetof(GameRegistryHeader, checksum);
    for (size_t i = 0; i < sizeof(GameRegistryHeader); i++) {
        if (i >= skip_offset && i < skip_offset + 4) continue;
        hash ^= p[i];
        hash *= 16777619U;
    }
    return hash;
}

static void test_recovery_from_valid_tmp_file(void) {
    mock_pspsdk_reset();

    char games_path[256], tmp_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(tmp_path, sizeof(tmp_path), "%s/db/%s", k_base_dir, GAMES_TMP);

    // Old games.dat with 1 entry
    GameRegistryHeader h_old = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 1,
        .next_uid = 2,
        .ready_flag = GAMEDIARY_MAGIC
    };
    h_old.checksum = calc_checksum(&h_old);
    mock_vfs_map_file(games_path, &h_old, sizeof(h_old));

    // Newer games.tmp with 2 entries and ready_flag set
    GameRegistryHeader h_new = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 2,
        .next_uid = 3,
        .ready_flag = GAMEDIARY_MAGIC
    };
    h_new.checksum = calc_checksum(&h_new);
    mock_vfs_map_file(tmp_path, &h_new, sizeof(h_new));

    // Calling storage_init() must detect valid games.tmp and promote it over games.dat
    storage_init(k_base_dir);

    // games.tmp must be gone
    ASSERT_FALSE(mock_vfs_file_exists(tmp_path));

    // games.dat must contain the newer header (2 entries)
    GameRegistryHeader h_loaded;
    mock_vfs_read_file(games_path, &h_loaded, sizeof(h_loaded));
    ASSERT_EQ(h_loaded.num_entries, 2U);
    ASSERT_EQ(h_loaded.next_uid, 3U);
}

static void test_recovery_from_corrupted_tmp_file(void) {
    mock_pspsdk_reset();

    char games_path[256], tmp_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(tmp_path, sizeof(tmp_path), "%s/db/%s", k_base_dir, GAMES_TMP);

    // Valid games.dat
    GameRegistryHeader h_valid = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 5,
        .next_uid = 6,
        .ready_flag = GAMEDIARY_MAGIC
    };
    h_valid.checksum = calc_checksum(&h_valid);
    mock_vfs_map_file(games_path, &h_valid, sizeof(h_valid));

    // Incomplete/corrupted games.tmp (ready_flag not set, bad checksum)
    GameRegistryHeader h_corrupted = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 6,
        .ready_flag = 0 // Crashed before finishing!
    };
    mock_vfs_map_file(tmp_path, &h_corrupted, sizeof(h_corrupted));

    storage_init(k_base_dir);

    // Corrupted tmp file must be cleaned up
    ASSERT_FALSE(mock_vfs_file_exists(tmp_path));

    // games.dat must be preserved intact (5 entries)
    GameRegistryHeader h_loaded;
    mock_vfs_read_file(games_path, &h_loaded, sizeof(h_loaded));
    ASSERT_EQ(h_loaded.num_entries, 5U);
}

static void test_recovery_from_backup_header(void) {
    mock_pspsdk_reset();

    char games_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);

    // Create file where primary header at offset 0 has corrupt magic
    GameRegistryHeader h_bad;
    memset(&h_bad, 0xFF, sizeof(h_bad));

    // Backup header at the end is intact
    GameRegistryHeader h_good = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 3,
        .next_uid = 4,
        .ready_flag = GAMEDIARY_MAGIC
    };
    h_good.checksum = calc_checksum(&h_good);

    u8 file_data[sizeof(GameRegistryHeader) * 2];
    memcpy(file_data, &h_bad, sizeof(h_bad));
    memcpy(file_data + sizeof(h_bad), &h_good, sizeof(h_good));
    mock_vfs_map_file(games_path, file_data, sizeof(file_data));

    storage_init(k_base_dir);

    // Registering a new game should succeed because backup header was recovered
    GameMetadata meta = {
        .category = CAT_PSP,
        .game_id = "ULUS-99999",
        .game_name = "Recovered Game"
    };
    u32 uid = 0;
    int res = storage_get_or_create_game(&meta, &uid);
    ASSERT_EQ(res, 0);
    // next_uid should be 4 (derived from recovered good header)
    ASSERT_EQ(uid, 4U);
}

void run_test_integration_storage_recovery(void) {
    TEST_SUITE_BEGIN("Integration: Storage Recovery & Resilience");
    TEST_RUN(test_recovery_from_valid_tmp_file);
    TEST_RUN(test_recovery_from_corrupted_tmp_file);
    TEST_RUN(test_recovery_from_backup_header);
    TEST_SUITE_END();
}
