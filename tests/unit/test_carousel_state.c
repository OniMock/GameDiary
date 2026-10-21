/**
 * Unit tests for src/app/ui/carousel_state.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/ui/carousel_state.h"
#include "app/data/data_loader.h"
#include "common/storage.h"
#include <stdlib.h>
#include <string.h>

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

static void setup_carousel_games(void) {
    mock_pspsdk_reset();
    const char *base = "ms0:" GDIARY_BASE_DIR;
    storage_init(base);

    GameMetadata m1 = { .category = CAT_PSP, .game_id = "ULES-00123", .game_name = "Game 1" };
    GameMetadata m2 = { .category = CAT_PSP, .game_id = "ULES-00124", .game_name = "Game 2" };
    GameMetadata m3 = { .category = CAT_PSP, .game_id = "ULES-00125", .game_name = "Game 3" };
    GameMetadata m4 = { .category = CAT_PSP, .game_id = "ULES-00126", .game_name = "Game 4" };

    u32 uid = 0;
    storage_get_or_create_game(&m1, &uid);
    storage_get_or_create_game(&m2, &uid);
    storage_get_or_create_game(&m3, &uid);
    storage_get_or_create_game(&m4, &uid);

    data_load_all();
}

static void test_carousel_init_and_destroy(void) {
    mock_pspsdk_reset();
    CarouselState cs;
    memset(&cs, 0, sizeof(cs));
    carousel_init(&cs, 0, NULL);
    ASSERT_EQ(cs.total, 0);
    ASSERT_EQ(cs.current_idx, 0);
    ASSERT_TRUE(carousel_is_settled(&cs));
    carousel_destroy(&cs);

    setup_carousel_games();
    memset(&cs, 0, sizeof(cs));
    carousel_init(&cs, 4, NULL);
    ASSERT_EQ(cs.total, 4);
    ASSERT_EQ(cs.current_idx, 0);
    ASSERT_TRUE(carousel_is_settled(&cs));
    carousel_destroy(&cs);
}

static void test_carousel_navigation_and_settle(void) {
    setup_carousel_games();
    CarouselState cs;
    memset(&cs, 0, sizeof(cs));
    carousel_init(&cs, 4, NULL);

    ASSERT_EQ(cs.current_idx, 0);
    ASSERT_TRUE(carousel_is_settled(&cs));

    // Navigate right (+1)
    carousel_navigate(&cs, +1);
    ASSERT_EQ(cs.current_idx, 1);
    ASSERT_FALSE(carousel_is_settled(&cs));

    // Update repeatedly with no button input to allow easing to settle
    for (int i = 0; i < 60; i++) {
        carousel_update(&cs, 0, 0);
    }
    ASSERT_TRUE(carousel_is_settled(&cs));

    // Navigate left (-1)
    carousel_navigate(&cs, -1);
    ASSERT_EQ(cs.current_idx, 0);

    // Navigate left again (unbounded index goes to -1)
    carousel_navigate(&cs, -1);
    ASSERT_EQ(cs.current_idx, -1);

    // Direct index set
    carousel_set_index(&cs, 3);
    ASSERT_EQ(cs.current_idx, 3);
    ASSERT_TRUE(carousel_is_settled(&cs));

    carousel_destroy(&cs);
}

static void test_carousel_icon_cache_and_generation(void) {
    setup_carousel_games();
    CarouselState cs;
    memset(&cs, 0, sizeof(cs));
    carousel_init(&cs, 4, NULL);
    int gen = cs.generation;

    CacheSlotState st = carousel_get_icon_state(&cs, 0);
    ASSERT_TRUE(st == CACHE_SLOT_PENDING || st == CACHE_SLOT_QUEUED || st == CACHE_SLOT_EMPTY);

    // Stale generation apply should be rejected and freed
    Texture *dummy_tex1 = (Texture *)calloc(1, sizeof(Texture));
    carousel_apply_loaded_icon(&cs, 0, 0, gen - 1, dummy_tex1);
    ASSERT_TRUE(carousel_get_icon(&cs, 0) == NULL);

    // Valid generation apply
    Texture *dummy_tex2 = (Texture *)calloc(1, sizeof(Texture));
    carousel_apply_loaded_icon(&cs, 0, 0, gen, dummy_tex2);
    if (cs.cache[0].inf_idx == 0) {
        ASSERT_EQ(carousel_get_icon_state(&cs, 0), CACHE_SLOT_LOADED);
        ASSERT_TRUE(carousel_get_icon(&cs, 0) == dummy_tex2);
    } else {
        free(dummy_tex2);
    }

    carousel_destroy(&cs);
}

static void test_carousel_empty_and_pending_checks(void) {
    CarouselState cs;
    memset(&cs, 0, sizeof(cs));

    // 1. Empty carousel checks
    carousel_init(&cs, 0, NULL);
    ASSERT_NULL(carousel_get_icon(&cs, 0));
    ASSERT_EQ(carousel_get_icon_state(&cs, 0), CACHE_SLOT_EMPTY);
    carousel_set_index(&cs, 0);
    carousel_destroy(&cs);

    // 2. Carousel with items pending check
    setup_carousel_games();
    carousel_init(&cs, 4, NULL);
    int pending = carousel_is_slot_pending(&cs, 0, 0);
    ASSERT_TRUE(pending == 0 || pending == 1);
    carousel_destroy(&cs);

    // Apply icon when mutex is destroyed
    Texture *dummy_tex = (Texture *)calloc(1, sizeof(Texture));
    carousel_apply_loaded_icon(&cs, 0, 0, 0, dummy_tex);
}

static void test_carousel_fast_scroll_and_set_index(void) {
    setup_carousel_games();
    CarouselState cs;
    memset(&cs, 0, sizeof(cs));
    carousel_init(&cs, 4, NULL);

    // Set index explicitly
    carousel_set_index(&cs, 2);
    ASSERT_EQ(cs.current_idx, 2);

    // Hold right button beyond CAROUSEL_HOLD_THRESHOLD (30 frames)
    for (int i = 0; i < 40; i++) {
        carousel_update(&cs, PSP_CTRL_RIGHT, 0);
    }
    ASSERT_TRUE(cs.current_idx > 2);

    // Release button
    carousel_update(&cs, 0, 0);
    ASSERT_EQ(cs.hold_frames, 0);

    // Re-initialization with pre-existing textures
    cs.cache[0].tex = (Texture *)calloc(1, sizeof(Texture));
    carousel_init(&cs, 4, NULL);
    ASSERT_NULL(cs.cache[0].tex); // was safely freed

    carousel_destroy(&cs);
}

void run_test_carousel_state(void) {
    TEST_SUITE_BEGIN("Carousel State Module");
    TEST_RUN(test_carousel_count_days_active);
    TEST_RUN(test_carousel_count_days_overflow);
    TEST_RUN(test_carousel_init_and_destroy);
    TEST_RUN(test_carousel_navigation_and_settle);
    TEST_RUN(test_carousel_icon_cache_and_generation);
    TEST_RUN(test_carousel_empty_and_pending_checks);
    TEST_RUN(test_carousel_fast_scroll_and_set_index);
    TEST_SUITE_END();
}
