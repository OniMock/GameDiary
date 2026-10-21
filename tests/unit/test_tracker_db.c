/**
 * Unit tests for src/plugin/tracker_db.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/tracker_db.h"
#include "common/db_schema.h"

static const char *k_tracker_path = "ms0:/PSP/COMMON/GameDiary/tracker.dat";

static void test_tracker_db_lifecycle(void) {
    mock_pspsdk_reset();
    tracker_db_set_path(k_tracker_path);

    // Initial check on non-existent file
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 0);

    // Add first entry
    int res = tracker_db_add("ULUS-10041");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 1);
    ASSERT_EQ(tracker_db_contains("ULES-00001"), 0);

    // Add second entry
    res = tracker_db_add("ULES-00001");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 1);
    ASSERT_EQ(tracker_db_contains("ULES-00001"), 1);

    // Duplicate add should be idempotent
    res = tracker_db_add("ULUS-10041");
    ASSERT_EQ(res, 0);

    // Verify header content
    TrackerHeader hdr;
    size_t rd = mock_vfs_read_file(k_tracker_path, &hdr, sizeof(hdr));
    ASSERT_EQ(rd, sizeof(hdr));
    ASSERT_EQ(hdr.magic, TRACKER_MAGIC);
    ASSERT_EQ(hdr.version, TRACKER_DB_VERSION);
    ASSERT_EQ(hdr.entry_count, 2U);

    // Remove first entry
    res = tracker_db_remove("ULUS-10041");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 0);
    ASSERT_EQ(tracker_db_contains("ULES-00001"), 1);

    // Remove non-existent entry
    res = tracker_db_remove("NON_EXISTENT");
    ASSERT_EQ(res, 0);

    // Remove last entry
    res = tracker_db_remove("ULES-00001");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("ULES-00001"), 0);

    rd = mock_vfs_read_file(k_tracker_path, &hdr, sizeof(hdr));
    ASSERT_EQ(rd, sizeof(hdr));
    ASSERT_EQ(hdr.entry_count, 0U);
}

static void test_tracker_db_legacy_format(void) {
    mock_pspsdk_reset();
    tracker_db_set_path(k_tracker_path);

    // Create legacy headerless file containing 2 raw entries
    TrackerEntry entries[2];
    memset(entries, 0, sizeof(entries));
    strncpy(entries[0].game_id, "LEGACY_1", sizeof(entries[0].game_id) - 1);
    strncpy(entries[1].game_id, "LEGACY_2", sizeof(entries[1].game_id) - 1);

    mock_vfs_map_file(k_tracker_path, entries, sizeof(entries));

    // Contains should work for legacy
    ASSERT_EQ(tracker_db_contains("LEGACY_1"), 1);
    ASSERT_EQ(tracker_db_contains("LEGACY_2"), 1);
    ASSERT_EQ(tracker_db_contains("OTHER"), 0);

    // Removing an entry upgrades/converts to standard versioned header
    int res = tracker_db_remove("LEGACY_1");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("LEGACY_1"), 0);
    ASSERT_EQ(tracker_db_contains("LEGACY_2"), 1);

    TrackerHeader hdr;
    mock_vfs_read_file(k_tracker_path, &hdr, sizeof(hdr));
    ASSERT_EQ(hdr.magic, TRACKER_MAGIC);
    ASSERT_EQ(hdr.entry_count, 1U);
}

static void test_tracker_db_errors_and_edge_cases(void) {
    mock_pspsdk_reset();

    // 1. Unset path
    tracker_db_set_path(NULL);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 0);
    ASSERT_EQ(tracker_db_add("ULUS-10041"), -1);
    ASSERT_EQ(tracker_db_remove("ULUS-10041"), -1);

    // 2. Invalid or NULL game IDs
    tracker_db_set_path(k_tracker_path);
    ASSERT_EQ(tracker_db_contains(NULL), 0);
    ASSERT_EQ(tracker_db_contains(""), 0);
    ASSERT_EQ(tracker_db_add(NULL), -1);
    ASSERT_EQ(tracker_db_add(""), -1);
    ASSERT_EQ(tracker_db_remove(NULL), -1);
    ASSERT_EQ(tracker_db_remove(""), -1);

    // 3. Corrupted header recovery in add
    TrackerHeader bad_hdr;
    memset(&bad_hdr, 0, sizeof(bad_hdr));
    bad_hdr.magic = 0xBAD00BAD;
    bad_hdr.version = 999;
    mock_vfs_map_file(k_tracker_path, &bad_hdr, sizeof(bad_hdr));

    int res = tracker_db_add("RECOVERED_ID");
    ASSERT_EQ(res, 0);
    ASSERT_EQ(tracker_db_contains("RECOVERED_ID"), 1);

    // 4. File I/O fault injection on add
    mock_vfs_fail_open(true);
    ASSERT_EQ(tracker_db_add("FAIL_GAME"), -1);
    mock_vfs_fail_open(false);

    // 5. Fail creating .tmp during remove
    mock_vfs_fail_path(".tmp");
    ASSERT_EQ(tracker_db_remove("RECOVERED_ID"), -1);
    mock_vfs_fail_path(NULL);

    // 6. Fail write during fresh entry creation
    mock_pspsdk_reset();
    mock_vfs_fail_write(true);
    ASSERT_EQ(tracker_db_add("FAIL_WRITE"), -1);
    mock_vfs_fail_write(false);
}

void run_test_tracker_db(void) {
    TEST_SUITE_BEGIN("Tracker Database (Blocklist) Module");
    TEST_RUN(test_tracker_db_lifecycle);
    TEST_RUN(test_tracker_db_legacy_format);
    TEST_RUN(test_tracker_db_errors_and_edge_cases);
    TEST_SUITE_END();
}
