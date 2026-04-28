/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

 /**
  * @file stats_calculator.c
  * @brief Stats calculator implementation.
  */

#include "app/data/stats_calculator.h"
#include "app/data/data_loader.h"
#include "common/utils.h"
#include "app/i18n/i18n.h"
#include "app/ui/ui_components.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int compare_period(const void* a, const void* b) {
    GameStats* ga = (GameStats*)a;
    GameStats* gb = (GameStats*)b;
    if (ga->period_playtime < gb->period_playtime) return 1;
    if (ga->period_playtime > gb->period_playtime) return -1;
    return 0;
}

static int compare_total(const void* a, const void* b) {
    GameStats* ga = (GameStats*)a;
    GameStats* gb = (GameStats*)b;
    if (ga->total_playtime < gb->total_playtime) return 1;
    if (ga->total_playtime > gb->total_playtime) return -1;
    return 0;
}

void stats_sort_by_period(void) {
    u32 count = data_get_game_count();
    GameStats* games = data_get_games();
    if (count > 0 && games) {
        qsort(games, count, sizeof(GameStats), compare_period);
        data_rebuild_uid_map();
    }
}

void stats_sort_by_total(void) {
    u32 count = data_get_game_count();
    GameStats* games = data_get_games();
    if (count > 0 && games) {
        qsort(games, count, sizeof(GameStats), compare_total);
        data_rebuild_uid_map();
    }
}

static void calc_weekly(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data) {
    u32 now_ts = utils_get_timestamp();
    time_t now_val = (time_t)now_ts;
    
    // apply offset
    now_val += query.offset * (7 * 86400);

    struct tm today_tm = *localtime(&now_val);
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    time_t today_start = mktime(&today_tm);
    
    out_data->column_count = 7;
    memset(out_data->column_values, 0, sizeof(out_data->column_values));
    
    for (int i = 0; i < 7; i++) {
        out_data->column_dates[i] = today_start - ((6 - i) * 86400);
    }

    time_t window_start = today_start - 6 * 86400;
    time_t window_end   = today_start + 86400;

    /* Split each session's duration across the 7-day columns.
     * A session spanning midnight (e.g. 23:00–01:00) contributes 1h to each
     * affected day instead of 2h to the day of the timestamp. */
    for (int i = 0; i < count; i++) {
        time_t s_start = (time_t)sessions[i].timestamp;
        time_t s_end   = s_start + (time_t)sessions[i].duration;

        /* Quick cull: skip sessions with no overlap with the 7-day window */
        if (s_end <= window_start || s_start >= window_end) continue;

        for (int col = 0; col < 7; col++) {
            time_t day_start = (time_t)out_data->column_dates[col];
            time_t day_end   = day_start + 86400;
            out_data->column_values[col] += utils_time_overlap_secs(s_start, s_end, day_start, day_end);
        }
    }
    
    u32 total_time = 0;
    u32 max_time = 3600; // Min scale 1h
    for (int i = 0; i < 7; i++) {
        total_time += out_data->column_values[i];
        if (out_data->column_values[i] > max_time) max_time = out_data->column_values[i];
    }
    out_data->max_value = max_time;

    struct tm start_d = *localtime(&out_data->column_dates[0]);
    struct tm end_d = *localtime(&out_data->column_dates[6]);
    char b1[16], b2[16];
    strftime(b1, sizeof(b1), i18n_get(MSG_DATE_FORMAT_SHORT), &start_d);
    strftime(b2, sizeof(b2), i18n_get(MSG_DATE_FORMAT_SHORT), &end_d);
    snprintf(out_data->context_title, sizeof(out_data->context_title), "%s - %s %d", b1, b2, end_d.tm_year + 1900);

    char dur_buf[32];
    u32 h = total_time / 3600;
    u32 m = (total_time % 3600) / 60;
    if (h > 0) snprintf(dur_buf, sizeof(dur_buf), "%s: %uh %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)h, (unsigned int)m);
    else snprintf(dur_buf, sizeof(dur_buf), "%s: %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)m);

    snprintf(out_data->context_subtitle, sizeof(out_data->context_subtitle), "%s", dur_buf);
}

static void calc_monthly(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data) {
    u32 now_ts = utils_get_timestamp();
    
    time_t today_val = (time_t)now_ts;
    struct tm today_tm = *localtime(&today_val);
    today_tm.tm_hour = 0; today_tm.tm_min = 0; today_tm.tm_sec = 0;
    time_t today_start = mktime(&today_tm);

    /* For rolling 30-day view, offset applies in steps of 30 days */
    time_t window_end = today_start + 86400 + (query.offset * (30 * 86400));
    time_t window_start = window_end - (30 * 86400);

    out_data->column_count = 30;
    memset(out_data->column_values, 0, sizeof(out_data->column_values));
    
    for (int i = 0; i < 30; i++) {
        out_data->column_dates[i] = window_start + (i * 86400);
    }

    /* Accumulate session time across the 30 columns */
    for (int i = 0; i < count; i++) {
        time_t s_start = (time_t)sessions[i].timestamp;
        time_t s_end   = s_start + (time_t)sessions[i].duration;

        if (s_end <= window_start || s_start >= window_end) continue;

        for (int d = 0; d < 30; d++) {
            time_t day_start = (time_t)out_data->column_dates[d];
            time_t day_end   = day_start + 86400;
            out_data->column_values[d] += utils_time_overlap_secs(s_start, s_end, day_start, day_end);
        }
    }
    
    u32 total_time = 0;
    u32 total_m1 = 0, total_m2 = 0;
    int m1 = -1, m2 = -1;
    int year1 = -1, year2 = -1;

    u32 max_time = 3600; 
    for (int i = 0; i < 30; i++) {
        total_time += out_data->column_values[i];
        if (out_data->column_values[i] > max_time) max_time = out_data->column_values[i];

        /* Calculate split totals for subtitle */
        struct tm dtm = *localtime(&out_data->column_dates[i]);
        if (m1 == -1) { 
            m1 = dtm.tm_mon; 
            year1 = dtm.tm_year + 1900;
        } else if (dtm.tm_mon != m1 && m2 == -1) {
            m2 = dtm.tm_mon;
            year2 = dtm.tm_year + 1900;
        }

        if (dtm.tm_mon == m1) total_m1 += out_data->column_values[i];
        else if (dtm.tm_mon == m2) total_m2 += out_data->column_values[i];
    }
    out_data->max_value = max_time;

    /* Context Title: Handle single month or overlapping months (e.g. Apr/May 2026) */
    if (m2 == -1) {
        snprintf(out_data->context_title, sizeof(out_data->context_title), "%s %d",
                 i18n_get(MSG_MONTH_JAN + m1), year1);
    } else {
        snprintf(out_data->context_title, sizeof(out_data->context_title), "%s/%s %d",
                 i18n_get(MSG_MONTH_JAN + m1), i18n_get(MSG_MONTH_JAN + m2), year2);
    }
    
    char dur_buf[96];
    if (m2 == -1) {
        char total_str[32];
        ui_format_duration(total_time, total_str, sizeof(total_str));
        snprintf(dur_buf, sizeof(dur_buf), "%s: %s", i18n_get(MSG_STATS_TOTAL_PLAYTIME), total_str);
    } else {
        char s1[32], s2[32];
        ui_format_duration(total_m1, s1, sizeof(s1));
        ui_format_duration(total_m2, s2, sizeof(s2));

        const char *mn1 = i18n_get(MSG_MONTH_JAN + m1);
        const char *mn2 = i18n_get(MSG_MONTH_JAN + m2);
        
        /* Format: "Total: (Apr) 5h 20m (May) 10h 5m" */
        snprintf(dur_buf, sizeof(dur_buf), "%s: (%s) %s (%s) %s", 
                 i18n_get(MSG_STATS_TOTAL_PLAYTIME), mn1, s1, mn2, s2);
    }

    snprintf(out_data->context_subtitle, sizeof(out_data->context_subtitle), "%s", dur_buf);
}

static void calc_yearly(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data) {
    u32 now_ts = utils_get_timestamp();
    time_t now_val = (time_t)now_ts;
    
    struct tm tm_info = *localtime(&now_val);
    int current_year = tm_info.tm_year + 1900 + (query.offset * 10);
    
    int start_year = current_year - 9;
    
    out_data->column_count = 10;
    memset(out_data->column_values, 0, sizeof(out_data->column_values));
    
    for (int i = 0; i < 10; i++) {
        struct tm ytm = {0};
        ytm.tm_year = (start_year + i) - 1900;
        ytm.tm_mon = 0; ytm.tm_mday = 1; ytm.tm_hour=0;ytm.tm_min=0;ytm.tm_sec=0;
        out_data->column_dates[i] = mktime(&ytm);
    }
    
    /* Precompute the exclusive end timestamp of each year column so we can use
     * time_overlap_secs. year_ends[i] = Jan 1 of (start_year + i + 1). */
    time_t year_ends[10];
    for (int i = 0; i < 9; i++) {
        year_ends[i] = out_data->column_dates[i + 1];
    }
    {
        struct tm nytm = {0};
        nytm.tm_year = current_year + 1 - 1900; /* year after the last column */
        nytm.tm_mon = 0; nytm.tm_mday = 1;
        year_ends[9] = mktime(&nytm);
    }

    /* Split each session across year columns (handles rare New Year's Eve
     * marathons that cross into the next year). */
    for (int i = 0; i < count; i++) {
        time_t s_start = (time_t)sessions[i].timestamp;
        time_t s_end   = s_start + (time_t)sessions[i].duration;

        /* Quick cull: outside the entire 10-year range */
        if (s_end <= out_data->column_dates[0] || s_start >= year_ends[9]) continue;

        for (int col = 0; col < 10; col++) {
            out_data->column_values[col] += utils_time_overlap_secs(
                s_start, s_end, out_data->column_dates[col], year_ends[col]);
        }
    }
    
    u32 total_time = 0;
    u32 max_time = 3600 * 10; // 10 hour min scale
    for (int i = 0; i < 10; i++) {
        total_time += out_data->column_values[i];
        if (out_data->column_values[i] > max_time) max_time = out_data->column_values[i];
    }
    out_data->max_value = max_time;

    snprintf(out_data->context_title, sizeof(out_data->context_title), "%d - %d", start_year, current_year);

    char dur_buf[32];
    u32 h = total_time / 3600;
    u32 m = (total_time % 3600) / 60;
    if (h > 0) snprintf(dur_buf, sizeof(dur_buf), "%s: %uh %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)h, (unsigned int)m);
    else snprintf(dur_buf, sizeof(dur_buf), "%s: %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)m);

    snprintf(out_data->context_subtitle, sizeof(out_data->context_subtitle), "%s", dur_buf);
}

static void calc_last_12_months(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data) {
    u32 now_ts = utils_get_timestamp();
    time_t now_val = (time_t)now_ts;

    // Apply offset (in steps of 12 months)
    struct tm cur_tm = *localtime(&now_val);
    cur_tm.tm_year += query.offset;
    cur_tm.tm_hour = 0; cur_tm.tm_min = 0; cur_tm.tm_sec = 0; cur_tm.tm_mday = 1;

    int cm = cur_tm.tm_mon;
    int cy = cur_tm.tm_year;

    out_data->column_count = 12;
    memset(out_data->column_values, 0, sizeof(out_data->column_values));

    time_t month_starts[12];
    time_t month_ends[12];

    for (int i = 0; i < 12; i++) {
        int rm = (cm - 11) + i;
        int target_m = rm;
        int target_y = cy;

        if (rm < 0) {
            target_m = rm + 12;
            target_y = cy - 1;
        }

        struct tm stm = {0};
        stm.tm_mon = target_m;
        stm.tm_year = target_y;
        stm.tm_mday = 1;
        month_starts[i] = mktime(&stm);
        out_data->column_dates[i] = month_starts[i];

        // End is start of next month
        struct tm etm = stm;
        etm.tm_mon++;
        if (etm.tm_mon > 11) {
            etm.tm_mon = 0;
            etm.tm_year++;
        }
        month_ends[i] = mktime(&etm);
    }

    /* Accumulate session time across the 12 columns */
    for (int i = 0; i < count; i++) {
        time_t s_start = (time_t)sessions[i].timestamp;
        time_t s_end   = s_start + (time_t)sessions[i].duration;

        if (s_end <= month_starts[0] || s_start >= month_ends[11]) continue;

        for (int m = 0; m < 12; m++) {
            out_data->column_values[m] += utils_time_overlap_secs(s_start, s_end, month_starts[m], month_ends[m]);
        }
    }

    u32 total_time = 0;
    u32 max_time = 3600 * 5; // 5 hour min scale
    for (int i = 0; i < 12; i++) {
        total_time += out_data->column_values[i];
        if (out_data->column_values[i] > max_time) max_time = out_data->column_values[i];
    }
    out_data->max_value = max_time;

    /* Context Title: "May 2025 - Apr 2026" */
    struct tm start_tm = *localtime(&month_starts[0]);
    struct tm end_tm = *localtime(&month_starts[11]);
    snprintf(out_data->context_title, sizeof(out_data->context_title), "%s %d - %s %d",
             i18n_get(MSG_MONTH_JAN + start_tm.tm_mon), start_tm.tm_year + 1900,
             i18n_get(MSG_MONTH_JAN + end_tm.tm_mon), end_tm.tm_year + 1900);

    char total_str[32];
    ui_format_duration(total_time, total_str, sizeof(total_str));
    snprintf(out_data->context_subtitle, sizeof(out_data->context_subtitle), "%s: %s", i18n_get(MSG_STATS_TOTAL_PLAYTIME), total_str);
}


void stats_calc_query(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data) {
    memset(out_data, 0, sizeof(StatsGraphData));
    out_data->query = query;
    if (!sessions || count == 0) return;

    switch (query.period) {
        case STATS_PERIOD_WEEKLY:
            calc_weekly(sessions, count, query, out_data);
            break;
        case STATS_PERIOD_MONTHLY:
            calc_monthly(sessions, count, query, out_data);
            break;
        case STATS_PERIOD_LAST_12_MONTHS:
            calc_last_12_months(sessions, count, query, out_data);
            break;
        case STATS_PERIOD_YEARLY:
            calc_yearly(sessions, count, query, out_data);
            break;
        default:
            break;
    }
}
