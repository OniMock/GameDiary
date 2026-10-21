/**
 * Integration Test: Plugin Tracker Pipeline
 * Simulates boot -> metadata -> config -> tracker runtime -> session log -> hotkey toggle
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "common/models.h"
#include "plugin/tracker_runtime.h"
#include "plugin/tracker_db.h"
#include "plugin/input_combo.h"
#include "plugin/plugin_config.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static void test_tracker_pipeline_flow(void) {
    mock_pspsdk_reset();
    mock_rtc_set_unix_time(1700000000U);

    // 1. Initialize plugin config
    plugin_config_init();

    // 2. Initialize storage
    storage_init(k_base_dir);

    // 3. Resolve metadata (mock GTA: LCS)
    GameMetadata meta = {
        .category = CAT_PSP,
        .game_id = "ULUS-10041",
        .game_name = "Grand Theft Auto: LCS",
        .apitype_str = "0x120",
        .file_path = "ms0:/ISO/GTA.ISO"
    };

    // 4. Initialize tracker runtime with hotkey active
    TrackerRuntime rt;
    tracker_runtime_init(&rt, k_base_dir, &meta, 1);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);

    // 5. Register game in storage
    u32 game_uid = 0;
    int res = storage_get_or_create_game(&meta, &game_uid);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(game_uid, 1U);

    // 6. Simulate 60s playtime accumulation and periodic flush
    SceOff session_offset = -1;
    u32 session_start_ts = mock_rtc_get_unix_time();
    mock_rtc_advance_seconds(60);

    u32 total_seconds = 60;
    res = storage_log_session(game_uid, total_seconds, session_start_ts, &session_offset);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(session_offset, 0); // Appended at byte 0

    // 7. Simulate another 60s -> in-place update
    mock_rtc_advance_seconds(60);
    total_seconds += 60; // 120s
    res = storage_log_session(game_uid, total_seconds, session_start_ts, &session_offset);
    ASSERT_EQ(res, 0);
    ASSERT_EQ(session_offset, 0); // Maintained at byte 0

    // Verify session entry on disk
    char s_path[256];
    snprintf(s_path, sizeof(s_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);
    SessionEntry entry;
    size_t rd = mock_vfs_read_file(s_path, &entry, sizeof(entry));
    ASSERT_EQ(rd, sizeof(entry));
    ASSERT_EQ(entry.game_uid, 1U);
    ASSERT_EQ(entry.duration, 120U);
    ASSERT_EQ(entry.timestamp, session_start_ts);

    // 8. User triggers hotkey toggle (L + R + SELECT held for 2s)
    int toggled = tracker_runtime_toggle(&rt, &meta);
    ASSERT_EQ(toggled, 1);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 0);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 1);

    // 9. Time passes while blocked -> no new session logging should happen
    mock_rtc_advance_seconds(300);

    // 10. User unblocks via hotkey toggle
    toggled = tracker_runtime_toggle(&rt, &meta);
    ASSERT_EQ(toggled, 1);
    ASSERT_EQ(tracker_runtime_is_tracking(&rt), 1);
    ASSERT_EQ(tracker_db_contains("ULUS-10041"), 0);
}

void run_test_integration_tracker_pipeline(void) {
    TEST_SUITE_BEGIN("Integration: Plugin Tracker Pipeline");
    TEST_RUN(test_tracker_pipeline_flow);
    TEST_SUITE_END();
}
