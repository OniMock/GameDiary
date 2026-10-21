/**
 * Unit tests for src/app/data/stats_calculator.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/data/stats_calculator.h"
#include "app/data/data_loader.h"
#include "app/i18n/i18n.h"

static void test_stats_calc_weekly(void) {
    mock_pspsdk_reset();
    i18n_set_language(LANG_EN);

    // Set fixed time: 2026-04-20 12:00:00 UTC (timestamp: 1776686400)
    mock_rtc_set_unix_time(1776686400U);

    // Create session: starts today at 10:00:00 (1776679200), duration = 3600 (1 hour)
    SessionEntry sessions[2];
    memset(sessions, 0, sizeof(sessions));
    sessions[0].game_uid = 1;
    sessions[0].timestamp = 1776679200U;
    sessions[0].duration = 3600;

    // Session 2: yesterday, duration = 1800 (30 mins)
    sessions[1].game_uid = 1;
    sessions[1].timestamp = 1776679200U - 86400;
    sessions[1].duration = 1800;

    StatsQuery q = {
        .period = STATS_PERIOD_WEEKLY,
        .offset = 0,
        .filter_uid = 0
    };

    StatsGraphData data;
    stats_calc_query(sessions, 2, q, &data);

    ASSERT_EQ(data.column_count, 7);
    // Last column (column 6) is today -> exactly 3600s
    ASSERT_EQ(data.column_values[6], 3600U);
    // Previous column (column 5) is yesterday -> exactly 1800s
    ASSERT_EQ(data.column_values[5], 1800U);
    // Column 0..4 have 0 playtime
    ASSERT_EQ(data.column_values[0], 0U);
    ASSERT_EQ(data.column_values[1], 0U);
    ASSERT_EQ(data.column_values[2], 0U);
    ASSERT_EQ(data.column_values[3], 0U);
    ASSERT_EQ(data.column_values[4], 0U);
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
}

void run_test_stats_calculator(void) {
    TEST_SUITE_BEGIN("Stats Calculator Module");
    TEST_RUN(test_stats_calc_weekly);
    TEST_RUN(test_stats_calc_monthly_and_yearly);
    TEST_SUITE_END();
}
