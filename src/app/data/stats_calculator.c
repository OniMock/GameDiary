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

    for (int i = 0; i < count; i++) {
        time_t s_time = (time_t)sessions[i].timestamp;
        if (s_time >= (today_start - 6 * 86400) && s_time < (today_start + 86400)) {
            int days_ago = (today_start >= s_time) ? ((today_start - s_time) / 86400 + 1) : 0;
            if (days_ago >= 0 && days_ago < 7) {
                int col_idx = 6 - days_ago;
                out_data->column_values[col_idx] += sessions[i].duration;
            }
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
    time_t now_val = (time_t)now_ts;
    
    struct tm tm_info = *localtime(&now_val);
    
    // Apply offset
    int new_month = tm_info.tm_mon + query.offset;
    tm_info.tm_year += new_month / 12;
    tm_info.tm_mon = new_month % 12;
    if (tm_info.tm_mon < 0) {
        tm_info.tm_mon += 12;
        tm_info.tm_year--;
    }
    tm_info.tm_mday = 1;
    tm_info.tm_hour = 0; tm_info.tm_min = 0; tm_info.tm_sec = 0;
    
    time_t month_start = mktime(&tm_info);
    
    // Calculate days in month
    int days_in_month = 31;
    if (tm_info.tm_mon == 3 || tm_info.tm_mon == 5 || tm_info.tm_mon == 8 || tm_info.tm_mon == 10) days_in_month = 30;
    else if (tm_info.tm_mon == 1) {
        int year = tm_info.tm_year + 1900;
        days_in_month = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 29 : 28;
    }
    
    out_data->column_count = days_in_month;
    memset(out_data->column_values, 0, sizeof(out_data->column_values));
    
    for (int i = 0; i < days_in_month; i++) {
        out_data->column_dates[i] = month_start + (i * 86400);
    }
    
    time_t month_end = out_data->column_dates[days_in_month - 1] + 86400;

    for (int i = 0; i < count; i++) {
        time_t s_time = (time_t)sessions[i].timestamp;
        if (s_time >= month_start && s_time < month_end) {
            int d = (s_time - month_start) / 86400;
            if (d >= 0 && d < days_in_month) {
                out_data->column_values[d] += sessions[i].duration;
            }
        }
    }
    
    u32 total_time = 0;
    u32 max_time = 3600; 
    for (int i = 0; i < days_in_month; i++) {
        total_time += out_data->column_values[i];
        if (out_data->column_values[i] > max_time) max_time = out_data->column_values[i];
    }
    out_data->max_value = max_time;

    snprintf(out_data->context_title, sizeof(out_data->context_title), "%s %d",
             i18n_get(MSG_MONTH_JAN + tm_info.tm_mon),
             tm_info.tm_year + 1900);
    
    char dur_buf[32];
    u32 h = total_time / 3600;
    u32 m = (total_time % 3600) / 60;
    if (h > 0) snprintf(dur_buf, sizeof(dur_buf), "%s: %uh %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)h, (unsigned int)m);
    else snprintf(dur_buf, sizeof(dur_buf), "%s: %um", i18n_get(MSG_STATS_TOTAL_PLAYTIME), (unsigned int)m);

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
    
    for (int i = 0; i < count; i++) {
        time_t s_time = (time_t)sessions[i].timestamp;
        struct tm s_tm = *localtime(&s_time);
        int sy = s_tm.tm_year + 1900;
        if (sy >= start_year && sy <= current_year) {
            out_data->column_values[sy - start_year] += sessions[i].duration;
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
        case STATS_PERIOD_YEARLY:
            calc_yearly(sessions, count, query, out_data);
            break;
        default:
            break;
    }
}
