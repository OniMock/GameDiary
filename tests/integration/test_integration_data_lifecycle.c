/**
 * Integration Test: Data Lifecycle (Plugin <-> App <-> Backup)
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "common/models.h"
#include "app/data/data_loader.h"
#include "app/data/data_backup.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static void test_end_to_end_data_lifecycle(void) {
    mock_pspsdk_reset();
    mock_rtc_set_unix_time(1700000000U);

    // 1. Plugin layer writes 2 games and 3 sessions to disk
    storage_init(k_base_dir);

    GameMetadata g1 = { .category = CAT_PSP, .game_id = "ULUS-10041", .game_name = "GTA" };
    GameMetadata g2 = { .category = CAT_PS1, .game_id = "SLUS-00067", .game_name = "Castlevania" };

    u32 uid1 = 0, uid2 = 0;
    storage_get_or_create_game(&g1, &uid1);
    storage_get_or_create_game(&g2, &uid2);

    SceOff off1 = -1, off2 = -1, off3 = -1;
    storage_log_session(uid1, 1800, 1700000000U, &off1);
    storage_log_session(uid1, 3600, 1700010000U, &off2);
    storage_log_session(uid2, 2400, 1700020000U, &off3);

    // 2. App layer loads all data
    int res = data_load_all();
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 2U);
    ASSERT_EQ(data_get_session_count(), 3U);

    // Compute stats
    data_calculate_stats(0, 2000000000U);
    GameStats *games = data_get_games();
    ASSERT_EQ(games[0].total_playtime, 5400U); // 1800 + 3600
    ASSERT_EQ(games[1].total_playtime, 2400U);

    // 3. Export to JSON backup
    res = data_backup_export();
    ASSERT_EQ(res, 0);

    // 4. Delete game 1
    res = data_delete_game_at_index(0);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(data_get_game_count(), 1U);
    ASSERT_EQ(data_get_session_count(), 1U); // Remaining session belongs to Game 2

    // 5. Restore from backup
    res = data_backup_import();
    ASSERT_EQ(res, 0);

    // Reload
    data_free();
    res = data_load_all();
    ASSERT_EQ(res, 0);

    // Verify both games and all 3 sessions restored!
    ASSERT_EQ(data_get_game_count(), 2U);
    ASSERT_EQ(data_get_session_count(), 3U);

    data_calculate_stats(0, 2000000000U);
    games = data_get_games();
    ASSERT_STR_EQ(games[0].entry.game_id, "ULUS-10041");
    ASSERT_EQ(games[0].total_playtime, 5400U);
    ASSERT_STR_EQ(games[1].entry.game_id, "SLUS-00067");
    ASSERT_EQ(games[1].total_playtime, 2400U);

    data_free();
}

void run_test_integration_data_lifecycle(void) {
    TEST_SUITE_BEGIN("Integration: End-to-End Data Lifecycle");
    TEST_RUN(test_end_to_end_data_lifecycle);
    TEST_SUITE_END();
}
