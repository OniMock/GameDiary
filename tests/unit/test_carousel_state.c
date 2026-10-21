/**
 * Unit tests for src/app/ui/carousel_state.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/ui/carousel_state.h"

static void test_carousel_count_days_active(void) {
    SessionEntry sessions[6];
    memset(sessions, 0, sizeof(sessions));

    // Day 1: timestamp 100000 (day = 100000 / 86400 = 1)
    sessions[0].game_uid = 10;
    sessions[0].timestamp = 100000;
    sessions[0].duration = 1800;

    // Day 1: timestamp 150000 (same day = 150000 / 86400 = 1)
    sessions[1].game_uid = 10;
    sessions[1].timestamp = 150000;
    sessions[1].duration = 3600;

    // Day 2: timestamp 200000 (day = 200000 / 86400 = 2)
    sessions[2].game_uid = 10;
    sessions[2].timestamp = 200000;
    sessions[2].duration = 600;

    // Day 3: different game UID 20 (day = 260000 / 86400 = 3)
    sessions[3].game_uid = 20;
    sessions[3].timestamp = 260000;
    sessions[3].duration = 1200;

    // Day 3: timestamp 270000 for game 10
    sessions[4].game_uid = 10;
    sessions[4].timestamp = 270000;
    sessions[4].duration = 1800;

    // Test for game UID 10: days are 1, 2, 3 -> exactly 3 unique days
    int days10 = carousel_count_days_active(sessions, 5, 10);
    ASSERT_EQ(days10, 3);

    // Test for game UID 20: exactly 1 day
    int days20 = carousel_count_days_active(sessions, 5, 20);
    ASSERT_EQ(days20, 1);

    // Test for non-existent game UID
    int days99 = carousel_count_days_active(sessions, 5, 99);
    ASSERT_EQ(days99, 0);

    // Test empty count
    ASSERT_EQ(carousel_count_days_active(sessions, 0, 10), 0);
}

static void test_carousel_count_days_overflow(void) {
    // MAX_TRACKED_DAYS is 128. If there are 129 unique days, it should return -1.
    SessionEntry sessions[130];
    for (int i = 0; i < 130; i++) {
        sessions[i].game_uid = 5;
        sessions[i].timestamp = (u32)(i * 86400 + 100);
        sessions[i].duration = 60;
    }

    // 128 days tracked -> returns 128
    int res128 = carousel_count_days_active(sessions, 128, 5);
    ASSERT_EQ(res128, 128);

    // 129 days tracked -> exceeds buffer -> returns -1
    int res129 = carousel_count_days_active(sessions, 129, 5);
    ASSERT_EQ(res129, -1);
}

void run_test_carousel_state(void) {
    TEST_SUITE_BEGIN("Carousel State Module");
    TEST_RUN(test_carousel_count_days_active);
    TEST_RUN(test_carousel_count_days_overflow);
    TEST_SUITE_END();
}
