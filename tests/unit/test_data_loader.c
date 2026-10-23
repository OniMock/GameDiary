/**
 * Unit tests for src/app/data/data_loader.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/data/data_loader.h"
#include "common/db_schema.h"
#include "common/models.h"
#include <time.h>

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static u32 calc_hdr_checksum(const GameRegistryHeader *h) {
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

static void setup_mock_database(void) {
    mock_pspsdk_reset();

    char games_path[256], sessions_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

    // Build games.dat with 2 games
    GameRegistryHeader h;
    memset(&h, 0, sizeof(h));
    h.magic = GAMEDIARY_MAGIC;
    h.version = DB_VERSION;
    h.num_entries = 2;
    h.next_uid = 3;
    h.ready_flag = GAMEDIARY_MAGIC;
    h.checksum = calc_hdr_checksum(&h);

    GameEntry entries[2];
    memset(entries, 0, sizeof(entries));

    entries[0].uid = 1;
    strncpy(entries[0].game_id, "ULUS-10041", sizeof(entries[0].game_id) - 1);
    strncpy(entries[0].game_name, "Grand Theft Auto", sizeof(entries[0].game_name) - 1);
    entries[0].category = CAT_PSP;

    entries[1].uid = 2;
    strncpy(entries[1].game_id, "SLUS-00067", sizeof(entries[1].game_id) - 1);
    strncpy(entries[1].game_name, "Castlevania SotN", sizeof(entries[1].game_name) - 1);
    entries[1].category = CAT_PS1;

    u8 g_buf[sizeof(GameRegistryHeader) + sizeof(entries) + sizeof(GameRegistryHeader)];
    memcpy(g_buf, &h, sizeof(h));
    memcpy(g_buf + sizeof(h), entries, sizeof(entries));
    memcpy(g_buf + sizeof(h) + sizeof(entries), &h, sizeof(h)); // backup header
    mock_vfs_map_file(games_path, g_buf, sizeof(g_buf));

    // Build sessions.dat with 3 sessions
    SessionEntry sessions[3];
    memset(sessions, 0, sizeof(sessions));

    // Game 1: 1800s at t=100000
    sessions[0].game_uid = 1;
    sessions[0].duration = 1800;
    sessions[0].timestamp = 100000;

    // Game 1: 3600s at t=200000
    sessions[1].game_uid = 1;
    sessions[1].duration = 3600;
    sessions[1].timestamp = 200000;

    // Game 2: 1200s at t=150000
    sessions[2].game_uid = 2;
    sessions[2].duration = 1200;
    sessions[2].timestamp = 150000;

    mock_vfs_map_file(sessions_path, sessions, sizeof(sessions));
}

static void test_data_loader_load_and_stats(void) {
    setup_mock_database();

    int res = data_load_all();
    ASSERT_EQ(res, 0);

    ASSERT_EQ(data_get_game_count(), 2U);
    ASSERT_EQ(data_get_session_count(), 3U);

    GameStats *games = data_get_games();
    ASSERT_NOT_NULL(games);
    ASSERT_EQ(games[0].entry.uid, 1U);
    ASSERT_STR_EQ(games[0].entry.game_id, "ULUS-10041");
    ASSERT_EQ(games[1].entry.uid, 2U);
    ASSERT_STR_EQ(games[1].entry.game_id, "SLUS-00067");

    // Calculate stats over all time (0 to 300000)
    data_calculate_stats(0, 300000);

    // Game 1: total playtime = 1800 + 3600 = 5400s, 2 sessions
    ASSERT_EQ(games[0].total_playtime, 5400U);
    ASSERT_EQ(games[0].session_count, 2U);
    // last_played_ts is session 1 end: 200000 + 3600 = 203600
    ASSERT_EQ(games[0].last_played_ts, 203600U);
    ASSERT_EQ(games[0].period_playtime, 5400U);

    // Game 2: total playtime = 1200s, 1 session
    ASSERT_EQ(games[1].total_playtime, 1200U);
    ASSERT_EQ(games[1].session_count, 1U);
    ASSERT_EQ(games[1].last_played_ts, 151200U);

    // Period filter: [120000, 180000] -> only session 2 (Game 2 at 150000) matches
    data_calculate_stats(120000, 180000);
    ASSERT_EQ(games[0].period_playtime, 0U);
    ASSERT_EQ(games[1].period_playtime, 1200U);

    data_free();
}

static void test_data_loader_delete_operations(void) {
    setup_mock_database();
    data_load_all();

    // Delete session 0 of Game 1 (timestamp 100000)
    int del_sess = data_delete_session(1, 100000);
    ASSERT_EQ(del_sess, 0);
    ASSERT_EQ(data_get_session_count(), 2U);

    // Delete non-existent session
    del_sess = data_delete_session(1, 999999);
    ASSERT_EQ(del_sess, -1);

    // Delete Game 1 (index 0)
    int del_game = data_delete_game_at_index(0);
    ASSERT_EQ(del_game, 0);
    ASSERT_EQ(data_get_game_count(), 1U);

    // Game 2 should now be at index 0 and its sessions preserved
    GameStats *remaining = data_get_games();
    ASSERT_EQ(remaining[0].entry.uid, 2U);
    ASSERT_STR_EQ(remaining[0].entry.game_id, "SLUS-00067");

    // Associated sessions for Game 1 must have been purged
    ASSERT_EQ(data_get_session_count(), 1U);
    SessionEntry *sess = data_get_sessions();
    ASSERT_EQ(sess[0].game_uid, 2U);

    // Delete remaining Game 2 (index 0) -> 0 games remain
    del_game = data_delete_game_at_index(0);
    ASSERT_EQ(del_game, 0);
    ASSERT_EQ(data_get_game_count(), 0U);
    ASSERT_NULL(data_get_games());
    ASSERT_EQ(data_get_session_count(), 0U);

    data_free();
}

static void test_data_loader_compute_game_details(void) {
    setup_mock_database();

    // Set time to 2026-04-20 12:00:00 (timestamp 1776686400)
    mock_rtc_set_unix_time(1776686400U);

    // Remap sessions for Game 1 at current timestamps
    char sessions_path[256];
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

    SessionEntry sessions[3];
    memset(sessions, 0, sizeof(sessions));

    // Session 1: Today (2 hours)
    sessions[0].game_uid = 1;
    sessions[0].timestamp = 1776686400U - 7200;
    sessions[0].duration = 7200;

    // Session 2: 3 days ago (1 hour)
    sessions[1].game_uid = 1;
    sessions[1].timestamp = 1776686400U - (3 * 86400);
    sessions[1].duration = 3600;

    // Session 3: 40 days ago (2 hours)
    sessions[2].game_uid = 1;
    sessions[2].timestamp = 1776686400U - (40 * 86400);
    sessions[2].duration = 7200;

    mock_vfs_map_file(sessions_path, sessions, sizeof(sessions));

    data_load_all();

    GameDetailStats details;
    data_compute_game_details(1, &details);

    // Week: Session 1 (7200) + Session 2 (3600) = 10800s
    ASSERT_EQ(details.playtime_week, 10800U);
    // Month: Session 1 (7200) + Session 2 (3600) = 10800s
    ASSERT_EQ(details.playtime_month, 10800U);
    // Year: All 3 sessions = 18000s
    ASSERT_EQ(details.playtime_year, 18000U);

    ASSERT_EQ(details.first_played, sessions[2].timestamp);
    ASSERT_EQ(details.last_played, sessions[0].timestamp + sessions[0].duration);

    data_free();
}

static void test_data_loader_errors_and_edge_cases(void) {
    mock_pspsdk_reset();

    // Calling before load
    ASSERT_EQ(data_delete_game_at_index(0), -1);
    ASSERT_EQ(data_delete_game_at_index(-1), -1);
    ASSERT_EQ(data_delete_session(1, 100), -1);

    // Load when games.dat does not exist -> returns -1
    int res = data_load_all();
    ASSERT_EQ(res, -1);
    ASSERT_EQ(data_get_game_count(), 0U);
    ASSERT_EQ(data_get_session_count(), 0U);

    // Calling delete on out-of-bounds index
    ASSERT_EQ(data_delete_game_at_index(0), -1);
    ASSERT_EQ(data_delete_session(1, 100), -1);

    data_free();
}

static void test_data_loader_corrupted_headers_and_recovery(void) {
    mock_pspsdk_reset();
    char games_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);

    // Case 1: Corrupted magic in primary and no backup
    GameRegistryHeader bad_h;
    memset(&bad_h, 0, sizeof(bad_h));
    bad_h.magic = 0xDEADBEEF;
    bad_h.version = DB_VERSION;
    bad_h.ready_flag = GAMEDIARY_MAGIC;
    mock_vfs_map_file(games_path, &bad_h, sizeof(bad_h));

    int res = data_load_all();
    ASSERT_EQ(res, -2);
    ASSERT_EQ(data_get_game_count(), 0U);
    ASSERT_NULL(data_get_games());

    // Case 2: Future DB version
    bad_h.magic = GAMEDIARY_MAGIC;
    bad_h.version = DB_VERSION + 10;
    mock_vfs_map_file(games_path, &bad_h, sizeof(bad_h));
    res = data_load_all();
    ASSERT_EQ(res, -2);

    // Case 3: Incomplete write (ready_flag != GAMEDIARY_MAGIC)
    bad_h.version = DB_VERSION;
    bad_h.ready_flag = 0;
    mock_vfs_map_file(games_path, &bad_h, sizeof(bad_h));
    res = data_load_all();
    ASSERT_EQ(res, -2);

    // Case 4: File smaller than header size
    u8 tiny_buf[10] = {0};
    mock_vfs_map_file(games_path, tiny_buf, sizeof(tiny_buf));
    res = data_load_all();
    ASSERT_EQ(res, -2);

    // Case 5: Corrupt num_entries exceeding file size
    bad_h.ready_flag = GAMEDIARY_MAGIC;
    bad_h.num_entries = 99999;
    mock_vfs_map_file(games_path, &bad_h, sizeof(bad_h));
    res = data_load_all();
    ASSERT_EQ(res, -2);
    ASSERT_EQ(data_get_game_count(), 0U);

    // Case 6: Primary header corrupted, but trailing backup header is valid -> Recovery succeeds
    GameRegistryHeader valid_h;
    memset(&valid_h, 0, sizeof(valid_h));
    valid_h.magic = GAMEDIARY_MAGIC;
    valid_h.version = DB_VERSION;
    valid_h.num_entries = 1;
    valid_h.next_uid = 2;
    valid_h.ready_flag = GAMEDIARY_MAGIC;
    valid_h.checksum = calc_hdr_checksum(&valid_h);

    GameEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.uid = 1;
    strncpy(entry.game_id, "TEST-00001", sizeof(entry.game_id) - 1);
    strncpy(entry.game_name, "Recovery Game", sizeof(entry.game_name) - 1);
    entry.category = CAT_PSP;

    u8 recovery_buf[sizeof(GameRegistryHeader) + sizeof(GameEntry) + sizeof(GameRegistryHeader)];
    bad_h.magic = 0xBAD00000;
    memcpy(recovery_buf, &bad_h, sizeof(bad_h));
    memcpy(recovery_buf + sizeof(bad_h), &entry, sizeof(entry));
    memcpy(recovery_buf + sizeof(bad_h) + sizeof(entry), &valid_h, sizeof(valid_h));

    mock_vfs_map_file(games_path, recovery_buf, sizeof(recovery_buf));
    res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 1U);
    GameStats *recovered_games = data_get_games();
    ASSERT_NOT_NULL(recovered_games);
    ASSERT_STR_EQ(recovered_games[0].entry.game_id, "TEST-00001");

    // Case 7: Calling stats and details on empty/null state should never crash
    data_free();
    data_calculate_stats(0, 100000);
    GameDetailStats details;
    data_compute_game_details(1, &details);
    ASSERT_EQ(details.playtime_week, 0U);
    ASSERT_EQ(details.playtime_month, 0U);
    ASSERT_EQ(details.playtime_year, 0U);

    data_free();
}

static void test_data_loader_session_file_variations(void) {
    mock_pspsdk_reset();
    char games_path[256], sessions_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

    // Build a valid games.dat with 1 game
    GameRegistryHeader h;
    memset(&h, 0, sizeof(h));
    h.magic = GAMEDIARY_MAGIC;
    h.version = DB_VERSION;
    h.num_entries = 1;
    h.next_uid = 2;
    h.ready_flag = GAMEDIARY_MAGIC;
    h.checksum = calc_hdr_checksum(&h);

    GameEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.uid = 1;
    strncpy(entry.game_id, "TEST-00001", sizeof(entry.game_id) - 1);
    strncpy(entry.game_name, "Single Game", sizeof(entry.game_name) - 1);
    entry.category = CAT_PSP;

    u8 g_buf[sizeof(GameRegistryHeader) + sizeof(GameEntry) + sizeof(GameRegistryHeader)];
    memcpy(g_buf, &h, sizeof(h));
    memcpy(g_buf + sizeof(h), &entry, sizeof(entry));
    memcpy(g_buf + sizeof(h) + sizeof(entry), &h, sizeof(h));
    mock_vfs_map_file(games_path, g_buf, sizeof(g_buf));

    // Case 1: sessions.dat exists but has size 0 (empty file)
    mock_vfs_map_file(sessions_path, "", 0);
    int res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 1U);
    ASSERT_EQ(data_get_session_count(), 0U);
    ASSERT_NULL(data_get_sessions());
    data_calculate_stats(0, 100000);
    data_rebuild_uid_map();

    // Case 2: sessions.dat has size > 0 but less than sizeof(SessionEntry) (5 bytes)
    mock_vfs_map_file(sessions_path, "12345", 5);
    res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_session_count(), 0U);
    ASSERT_NULL(data_get_sessions());

    // Case 3: sessions.dat has unaligned size: 1 valid SessionEntry (12 bytes) + 4 extra bytes = 16 bytes
    u8 unaligned_sess[sizeof(SessionEntry) + 4];
    memset(unaligned_sess, 0, sizeof(unaligned_sess));
    SessionEntry *s_ptr = (SessionEntry *)unaligned_sess;
    s_ptr->game_uid = 1;
    s_ptr->duration = 300;
    s_ptr->timestamp = 1000;
    mock_vfs_map_file(sessions_path, unaligned_sess, sizeof(unaligned_sess));

    res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_session_count(), 1U);
    ASSERT_NOT_NULL(data_get_sessions());
    data_calculate_stats(0, 2000);
    GameStats *g = data_get_games();
    ASSERT_NOT_NULL(g);
    ASSERT_EQ(g[0].total_playtime, 300U);

    // Case 4: Delete the last remaining session -> triggers free(g_sessions) and g_sessions = NULL
    int del_res = data_delete_session(1, 1000);
    ASSERT_EQ(del_res, 0);
    ASSERT_EQ(data_get_session_count(), 0U);
    ASSERT_NULL(data_get_sessions());

    // Case 5: Valid games.dat with num_entries == 0
    h.num_entries = 0;
    h.checksum = calc_hdr_checksum(&h);
    u8 empty_g_buf[sizeof(GameRegistryHeader) * 2];
    memcpy(empty_g_buf, &h, sizeof(h));
    memcpy(empty_g_buf + sizeof(h), &h, sizeof(h));
    mock_vfs_map_file(games_path, empty_g_buf, sizeof(empty_g_buf));

    res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 0U);
    ASSERT_NULL(data_get_games());
    data_calculate_stats(0, 100);
    data_rebuild_uid_map();

    // Case 6: Game entry with max_uid > 100000 (safeguard prevents huge uid_map allocation)
    h.num_entries = 1;
    h.checksum = calc_hdr_checksum(&h);
    entry.uid = 150000;
    memcpy(g_buf, &h, sizeof(h));
    memcpy(g_buf + sizeof(h), &entry, sizeof(entry));
    memcpy(g_buf + sizeof(h) + sizeof(entry), &h, sizeof(h));
    mock_vfs_map_file(games_path, g_buf, sizeof(g_buf));

    res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 1U);
    data_calculate_stats(0, 100);

    data_free();
}

static void test_data_loader_write_failure_paths(void) {
    mock_pspsdk_reset();
    setup_mock_database();

    int res = data_load_all();
    ASSERT_EQ(res, 0);

    // Case 1: Fail opening sessions.dat during delete write
    mock_vfs_fail_path("sessions.dat");
    int del_res = data_delete_game_at_index(0);
    ASSERT_EQ(del_res, -3);
    mock_vfs_fail_path(NULL);

    // Case 2: Fail opening games.dat during delete write
    setup_mock_database();
    res = data_load_all();
    ASSERT_EQ(res, 0);

    mock_vfs_fail_path("games.dat");
    del_res = data_delete_game_at_index(0);
    ASSERT_EQ(del_res, -2);
    mock_vfs_fail_path(NULL);

    data_free();
}

void run_test_data_loader(void) {
    TEST_SUITE_BEGIN("Data Loader & Query Module");
    TEST_RUN(test_data_loader_load_and_stats);
    TEST_RUN(test_data_loader_delete_operations);
    TEST_RUN(test_data_loader_compute_game_details);
    TEST_RUN(test_data_loader_errors_and_edge_cases);
    TEST_RUN(test_data_loader_corrupted_headers_and_recovery);
    TEST_RUN(test_data_loader_session_file_variations);
    TEST_RUN(test_data_loader_write_failure_paths);
    TEST_SUITE_END();
}
