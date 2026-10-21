/**
 * Unit tests for Tracker Time Accounting & Sleep Gap Heuristic (BUG-04 Verification)
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "common/storage.h"
#include "common/db_schema.h"
#include "common/models.h"
#include "common/utils.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

/**
 * Replicating tracker.c's exact time accounting state machine to evaluate
 * normal execution, sleep gap tolerance, and persistence behaviors.
 */
typedef struct {
    u32 pending_seconds;
    u32 session_total_seconds;
    SceOff current_session_offset;
    u32 current_game_uid;
    u32 session_start_ts;
    u32 last_tick_ts;
    int is_tracking;
} MockTrackerState;

static void mock_tracker_init(MockTrackerState *s, u32 game_uid, u32 start_ts) {
    memset(s, 0, sizeof(*s));
    s->current_game_uid = game_uid;
    s->session_start_ts = start_ts;
    s->last_tick_ts = start_ts;
    s->current_session_offset = -1;
    s->is_tracking = 1;
}

static void mock_tracker_flush_pending(MockTrackerState *s) {
    if (s->current_game_uid == 0 || !s->is_tracking) {
        s->pending_seconds = 0;
        return;
    }

    s->session_total_seconds += s->pending_seconds;
    s->pending_seconds = 0;

    if (s->session_total_seconds >= 60 || s->current_session_offset != -1) {
        storage_log_session(s->current_game_uid, s->session_total_seconds,
                            s->session_start_ts, &s->current_session_offset);
    }
}

static void mock_tracker_tick(MockTrackerState *s, u32 now_ts) {
    if (!s->is_tracking || s->current_game_uid == 0) return;

    u32 diff = now_ts - s->last_tick_ts;

    // Check same calendar day
    int same_day = ((s->session_start_ts / 86400U) == (now_ts / 86400U));
    if (!same_day) {
        // Midnight transition
        mock_tracker_flush_pending(s);
        s->session_start_ts = now_ts;
        s->session_total_seconds = 0;
        s->current_session_offset = -1;
        s->pending_seconds = 0;
    } else if (diff >= 10) {
        // Sleep resume: gap >= 10s
        mock_tracker_flush_pending(s);
    } else {
        s->pending_seconds++;
        if (s->pending_seconds >= 60) {
            s->session_total_seconds += s->pending_seconds;
            storage_log_session(s->current_game_uid, s->session_total_seconds,
                                s->session_start_ts, &s->current_session_offset);
            s->pending_seconds = 0;
        }
    }

    s->last_tick_ts = now_ts;
}

static void test_tracker_normal_accumulation(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    GameMetadata meta = { .category = CAT_PSP, .game_id = "ULUS-10041" };
    u32 uid = 0;
    storage_get_or_create_game(&meta, &uid);

    MockTrackerState state;
    // Noon UTC (12:00:00) so a 2h sleep does not cross midnight
    u32 start_ts = 1699963200U;
    mock_tracker_init(&state, uid, start_ts);

    // Simulate 65 consecutive 1-second ticks
    for (u32 i = 1; i <= 65; i++) {
        mock_tracker_tick(&state, start_ts + i);
    }

    // After 65 seconds: 60s flushed, 5s pending
    ASSERT_EQ(state.session_total_seconds, 60U);
    ASSERT_EQ(state.pending_seconds, 5U);
    ASSERT_TRUE(state.current_session_offset >= 0);

    // Check disk content
    char s_path[256];
    snprintf(s_path, sizeof(s_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);
    SessionEntry entry;
    size_t rd = mock_vfs_read_file(s_path, &entry, sizeof(entry));
    ASSERT_EQ(rd, sizeof(entry));
    ASSERT_EQ(entry.duration, 60U);
}

static void test_tracker_sleep_resume_preserves_accumulated_seconds(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    GameMetadata meta = { .category = CAT_PSP, .game_id = "ULUS-10041" };
    u32 uid = 0;
    storage_get_or_create_game(&meta, &uid);

    MockTrackerState state;
    // Noon UTC (12:00:00) so a 2h sleep does not cross midnight
    u32 start_ts = 1699963200U;
    mock_tracker_init(&state, uid, start_ts);

    // Play for 75 seconds (1 flush of 60s occurs, leaving 15s pending)
    for (u32 i = 1; i <= 75; i++) {
        mock_tracker_tick(&state, start_ts + i);
    }
    ASSERT_EQ(state.session_total_seconds, 60U);
    ASSERT_EQ(state.pending_seconds, 15U);

    // PSP goes to sleep for 2 hours (7200 seconds)
    u32 resume_ts = start_ts + 75 + 7200;
    mock_tracker_tick(&state, resume_ts); // diff = 7200 >= 10

    // BUG-04 analysis:
    // When diff >= 10, session_flush_pending() IS called:
    // session_total_seconds += pending_seconds (60 + 15 = 75s)
    // pending_seconds = 0
    // And storage_log_session updates the record on disk!
    ASSERT_EQ(state.session_total_seconds, 75U);
    ASSERT_EQ(state.pending_seconds, 0U);

    // Verify disk has 75 seconds, NOT 7275 seconds!
    char s_path[256];
    snprintf(s_path, sizeof(s_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);
    SessionEntry entry;
    size_t rd = mock_vfs_read_file(s_path, &entry, sizeof(entry));
    ASSERT_EQ(rd, sizeof(entry));
    ASSERT_EQ(entry.duration, 75U);
}

static void test_tracker_midnight_rollover(void) {
    mock_pspsdk_reset();
    storage_init(k_base_dir);

    GameMetadata meta = { .category = CAT_PSP, .game_id = "ULUS-10041" };
    u32 uid = 0;
    storage_get_or_create_game(&meta, &uid);

    // Day 1: 23:58:00 (ts = 86400 - 120 = 86280)
    u32 day1_start = 86280U;
    MockTrackerState state;
    mock_tracker_init(&state, uid, day1_start);

    // Play 119 seconds (until 23:59:59)
    for (u32 i = 1; i <= 119; i++) {
        mock_tracker_tick(&state, day1_start + i);
    }
    ASSERT_EQ(state.session_total_seconds, 60U);
    ASSERT_EQ(state.pending_seconds, 59U);

    // Next second is Day 2: 00:00:00 (ts = 86400)
    u32 day2_ts = 86400U;
    mock_tracker_tick(&state, day2_ts);

    // Day 1 session must have been flushed (60 + 59 = 119 seconds)
    // And state reset for Day 2!
    ASSERT_EQ(state.session_start_ts, day2_ts);
    ASSERT_EQ(state.session_total_seconds, 0U);
    ASSERT_EQ(state.pending_seconds, 0U);
}

void run_test_tracker_time_accounting(void) {
    TEST_SUITE_BEGIN("Tracker Time Accounting & Gap Handling Module");
    TEST_RUN(test_tracker_normal_accumulation);
    TEST_RUN(test_tracker_sleep_resume_preserves_accumulated_seconds);
    TEST_RUN(test_tracker_midnight_rollover);
    TEST_SUITE_END();
}
