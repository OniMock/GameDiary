/**
 * Unit tests for src/app/data/stats_calculator.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/data/stats_calculator.h"
#include "app/data/data_loader.h"
#include "app/i18n/i18n.h"
#include "common/db_schema.h"
#include "common/models.h"
#include <stddef.h>

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

static void setup_mock_db_for_sorting(void) {
    mock_pspsdk_reset();

    char games_path[256], sessions_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

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
    strncpy(entries[0].game_name, "Game A", sizeof(entries[0].game_name) - 1);
    entries[0].category = CAT_PSP;

    entries[1].uid = 2;
    strncpy(entries[1].game_id, "SLUS-00067", sizeof(entries[1].game_id) - 1);
    strncpy(entries[1].game_name, "Game B", sizeof(entries[1].game_name) - 1);
    entries[1].category = CAT_PS1;

    u8 g_buf[sizeof(GameRegistryHeader) + sizeof(entries) + sizeof(GameRegistryHeader)];
    memcpy(g_buf, &h, sizeof(h));
    memcpy(g_buf + sizeof(h), entries, sizeof(entries));
    memcpy(g_buf + sizeof(h) + sizeof(entries), &h, sizeof(h));
    mock_vfs_map_file(games_path, g_buf, sizeof(g_buf));

    // Sessions: Game 1 has 1000s, Game 2 has 5000s
    SessionEntry sessions[2];
    memset(sessions, 0, sizeof(sessions));

    sessions[0].game_uid = 1;
    sessions[0].duration = 1000;
    sessions[0].timestamp = 100000;

    sessions[1].game_uid = 2;
    sessions[1].duration = 5000;
    sessions[1].timestamp = 100000;

    mock_vfs_map_file(sessions_path, sessions, sizeof(sessions));
}

static void test_stats_calc_weekly(void) {
    mock_pspsdk_reset();
    i18n_set_language(LANG_EN);

    // Set fixed time: 2026-04-20 12:00:00 UTC (timestamp: 1776686400)
    mock_rtc_set_unix_time(1776686400U);

    // Create session: starts today at 10:00:00 (1776679200), duration = 3600 (1 hour)
    SessionEntry sessions[3];
    memset(sessions, 0, sizeof(sessions));
    sessions[0].game_uid = 1;
    sessions[0].timestamp = 1776679200U;
    sessions[0].duration = 3600;

    // Session 2: yesterday, duration = 1800 (30 mins)
    sessions[1].game_uid = 1;
    sessions[1].timestamp = 1776679200U - 86400;
    sessions[1].duration = 1800;

    // Session 3: outside 7-day window (10 days ago), should be culled
    sessions[2].game_uid = 1;
    sessions[2].timestamp = 1776679200U - (10 * 86400);
    sessions[2].duration = 1800;

    StatsQuery q = {
        .period = STATS_PERIOD_WEEKLY,
        .offset = 0,
        .filter_uid = 0
    };

    StatsGraphData data;
    stats_calc_query(sessions, 3, q, &data);

    ASSERT_EQ(data.column_count, 7);
    // Last column (column 6) is today -> exactly 3600s
    ASSERT_EQ(data.column_values[6], 3600U);
    // Previous column (column 5) is yesterday -> exactly 1800s
    ASSERT_EQ(data.column_values[5], 1800U);
    // Column 0..4 have 0 playtime
    ASSERT_EQ(data.column_values[0], 0U);

    // Test with filter_uid matching
    q.filter_uid = 1;
    stats_calc_query(sessions, 3, q, &data);
    ASSERT_EQ(data.column_values[6], 3600U);

    // Test with filter_uid mismatching
    q.filter_uid = 99;
    stats_calc_query(sessions, 3, q, &data);
    ASSERT_EQ(data.column_values[6], 0U);
}

static void test_stats_calc_monthly_and_yearly(void) {
    mock_pspsdk_reset();
    i18n_set_language(LANG_EN);
    mock_rtc_set_unix_time(1776686400U);

    SessionEntry session = { .game_uid = 1, .timestamp = 1776686400U - 3600, .duration = 1800 };

    // Monthly query: 30 columns
    StatsQuery q_month = { .period = STATS_PERIOD_MONTHLY, .offset = 0, .filter_uid = 0 };
    StatsGraphData d_month;
    stats_calc_query(&session, 1, q_month, &d_month);
    ASSERT_EQ(d_month.column_count, 30);

    // Yearly query: 10 columns
    StatsQuery q_year = { .period = STATS_PERIOD_YEARLY, .offset = 0, .filter_uid = 0 };
    StatsGraphData d_year;
    stats_calc_query(&session, 1, q_year, &d_year);
    ASSERT_EQ(d_year.column_count, 10);
    // The last column is current year -> should have 1800s
    ASSERT_EQ(d_year.column_values[9], 1800U);

    // Yearly with session far in past (20 years ago)
    SessionEntry old_session = { .game_uid = 1, .timestamp = 1776686400U - (20 * 365 * 86400), .duration = 1800 };
    stats_calc_query(&old_session, 1, q_year, &d_year);
    ASSERT_EQ(d_year.column_values[9], 0U);
}

static void test_stats_calc_last_12_months(void) {
    mock_pspsdk_reset();
    i18n_set_language(LANG_EN);
    // Set time to 2026-04-20
    mock_rtc_set_unix_time(1776686400U);

    SessionEntry sessions[3];
    memset(sessions, 0, sizeof(sessions));

    // Session 1: Current month (Apr 2026) -> 2 hours
    sessions[0].game_uid = 1;
    sessions[0].timestamp = 1776686400U - 7200;
    sessions[0].duration = 7200;

    // Session 2: 3 months ago (Jan 2026) -> 1 hour
    sessions[1].game_uid = 1;
    sessions[1].timestamp = 1776686400U - (90 * 86400);
    sessions[1].duration = 3600;

    // Session 3: 24 months ago (out of 12-month window)
    sessions[2].game_uid = 1;
    sessions[2].timestamp = 1776686400U - (730 * 86400);
    sessions[2].duration = 3600;

    StatsQuery q = {
        .period = STATS_PERIOD_LAST_12_MONTHS,
        .offset = 0,
        .filter_uid = 0
    };

    StatsGraphData data;
    stats_calc_query(sessions, 3, q, &data);

    ASSERT_EQ(data.column_count, 12);
    // The 12th column (index 11) is current month -> 7200s
    ASSERT_EQ(data.column_values[11], 7200U);

    // Test with filter_uid
    q.filter_uid = 99;
    stats_calc_query(sessions, 3, q, &data);
    ASSERT_EQ(data.column_values[11], 0U);
}

static void test_stats_sorting(void) {
    setup_mock_db_for_sorting();
    i18n_set_language(LANG_EN);

    int res = data_load_all();
    ASSERT_EQ(res, 0);

    data_calculate_stats(0, 500000);

    // Initial order: Game 1 has 1000s, Game 2 has 5000s
    GameStats *games = data_get_games();
    ASSERT_NOT_NULL(games);

    // Sort by total playtime descending (Game 2 should come first)
    stats_sort_by_total();
    games = data_get_games();
    ASSERT_EQ(games[0].entry.uid, 2);
    ASSERT_EQ(games[1].entry.uid, 1);

    // Sort by period playtime descending (Game 2 has 5000s, Game 1 has 1000s)
    stats_sort_by_period();
    games = data_get_games();
    ASSERT_EQ(games[0].entry.uid, 2);
    ASSERT_EQ(games[1].entry.uid, 1);

    // Test sorting with equal values
    games[0].total_playtime = 2000;
    games[1].total_playtime = 2000;
    stats_sort_by_total();

    games[0].period_playtime = 2000;
    games[1].period_playtime = 2000;
    stats_sort_by_period();
}

static void test_stats_calc_null_and_bounds(void) {
    StatsGraphData data;
    StatsQuery q = { .period = STATS_PERIOD_WEEKLY, .offset = 0, .filter_uid = 0 };

    // Null sessions or count = 0
    stats_calc_query(NULL, 0, q, &data);
    ASSERT_EQ(data.column_count, 0);

    SessionEntry s = { .game_uid = 1, .timestamp = 100, .duration = 10 };
    stats_calc_query(&s, 0, q, &data);
    ASSERT_EQ(data.column_count, 0);

    // Unknown period
    q.period = (StatsPeriod)999;
    stats_calc_query(&s, 1, q, &data);
    ASSERT_EQ(data.column_count, 0);
}

void run_test_stats_calculator(void) {
    TEST_SUITE_BEGIN("Stats Calculator Module");
    TEST_RUN(test_stats_calc_weekly);
    TEST_RUN(test_stats_calc_monthly_and_yearly);
    TEST_RUN(test_stats_calc_last_12_months);
    TEST_RUN(test_stats_sorting);
    TEST_RUN(test_stats_calc_null_and_bounds);
    TEST_SUITE_END();
}
