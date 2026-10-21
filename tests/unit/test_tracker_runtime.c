/**
 * Unit tests for src/plugin/tracker_runtime.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/tracker_runtime.h"
#include "plugin/tracker_db.h"
#include "common/db_schema.h"

static void test_metadata_ready(void) {
    ASSERT_EQ(tracker_runtime_metadata_ready(NULL), 0);

    GameMetadata m1 = {0};
    ASSERT_EQ(tracker_runtime_metadata_ready(&m1), 0);

    GameMetadata m2 = { .game_id = "UNKNOWN-00000", .category = CAT_PSP };
    ASSERT_EQ(tracker_runtime_metadata_ready(&m2), 0);

    GameMetadata m3 = { .game_id = "UNKNOWN-00000", .category = CAT_HOMEBREW };
    ASSERT_EQ(tracker_runtime_metadata_ready(&m3), 0);

    GameMetadata m4 = { .game_id = "ULUS-10041", .category = CAT_PSP };
    ASSERT_EQ(tracker_runtime_metadata_ready(&m4), 1);

    GameMetadata m5 = { .game_id = "HBX12345678", .category = CAT_HOMEBREW };
    ASSERT_EQ(tracker_runtime_metadata_ready(&m5), 1);

    // Disc launch with unknown game id
    GameMetadata m6 = { .game_id = "UNKNOWN-00000", .category = CAT_UNKNOWN, .file_path = "disc0:/PSP_GAME/SYSDIR/BOOT.BIN" };
    ASSERT_EQ(tracker_runtime_metadata_ready(&m6), 0);
}

static void test_tracker_runtime_lifecycle(void) {
    mock_pspsdk_reset();
    const char *base_dir = "ms0:/PSP/COMMON/GameDiary";

    TrackerRuntime rt;
    GameMetadata meta = { .category = CAT_PSP, .game_id = "ULUS-10041" };

    // 1. Hotkey disabled (honor_blocklist = 0) -> always active
    tracker_runtime_init(&rt, base_dir, &meta, 0);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_ACTIVE);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);

    // Toggle should be ignored when blocklist_active is false
    int tog = tracker_runtime_toggle(&rt, &meta);
    ASSERT_EQ(tog, 0);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);

    // 2. Hotkey enabled (honor_blocklist = 1), initially unblocked
    tracker_runtime_init(&rt, base_dir, &meta, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_ACTIVE);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);

    // Toggle -> should add to blocklist and become blocked
    tog = tracker_runtime_toggle(&rt, &meta);
    ASSERT_EQ(tog, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_BLOCKED);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 0);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 1);

    // Toggle again -> should unblock
    tog = tracker_runtime_toggle(&rt, &meta);
    ASSERT_EQ(tog, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_ACTIVE);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 0);

    // 3. Init with pre-blocked game
    tracker_db_add("ULUS-10041");
    tracker_runtime_init(&rt, base_dir, &meta, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_BLOCKED);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 0);

    // 4. Init with unresolved metadata
    GameMetadata meta_unresolved = { .category = CAT_PSP, .game_id = "UNKNOWN-00000" };
    tracker_runtime_init(&rt, base_dir, &meta_unresolved, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_NOT_READY);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 0);
}

static void test_tracker_runtime_defensive_checks(void) {
    mock_pspsdk_reset();

    // NULL pointer checks
    tracker_runtime_init(NULL, "ms0:/PSP/COMMON/GameDiary", NULL, 0);
    ASSERT_EQ(tracker_runtime_get_mode(NULL), TRACKER_RUNTIME_NOT_READY);
    ASSERT_EQ(tracker_runtime_is_tracking(NULL), 0);

    TrackerRuntime empty_rt;
    memset(&empty_rt, 0, sizeof(empty_rt));
    tracker_runtime_refresh_block(NULL);
    tracker_runtime_refresh_block(&empty_rt);

    // Refresh block when populated
    TrackerRuntime rt;
    GameMetadata meta = { .category = CAT_PSP, .game_id = "ULUS-10041" };
    tracker_runtime_init(&rt, "ms0:/PSP/COMMON/GameDiary", &meta, 1);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_ACTIVE);

    // Externally add to tracker db and refresh
    tracker_db_add("ULUS-10041");
    tracker_runtime_refresh_block(&rt);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_BLOCKED);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 0);

    tracker_db_remove("ULUS-10041");
    tracker_runtime_refresh_block(&rt);
    ASSERT_EQ(tracker_runtime_get_mode(&rt), TRACKER_RUNTIME_ACTIVE);

    // Toggle with invalid parameters
    ASSERT_EQ(tracker_runtime_toggle(NULL, &meta), 0);
    ASSERT_EQ(tracker_runtime_toggle(&rt, NULL), 0);
    GameMetadata unready_meta = { .category = CAT_PSP, .game_id = "UNKNOWN-00000" };
    ASSERT_EQ(tracker_runtime_toggle(&rt, &unready_meta), 0);
}

void run_test_tracker_runtime(void) {
    TEST_SUITE_BEGIN("Tracker Runtime State Module");
    TEST_RUN(test_metadata_ready);
    TEST_RUN(test_tracker_runtime_lifecycle);
    TEST_RUN(test_tracker_runtime_defensive_checks);
    TEST_SUITE_END();
}
