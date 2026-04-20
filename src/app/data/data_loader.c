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
  * @file data_loader.c
  * @brief Data loader implementation.
  */

#include "app/data/data_loader.h"
#include "common/utils.h"
#include <pspkernel.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

static GameStats *g_games = NULL;
static u32 g_game_count = 0;
static SessionEntry *g_sessions = NULL;
static u32 g_session_count = 0;

static u16 *g_uid_map = NULL;
static u32 g_uid_map_size = 0;

int data_load_all(void) {
    const char *prefix = utils_get_device_prefix();
    utils_ensure_storage_dirs(prefix);

    // 1. Load games.dat
    char path[256];
    snprintf(path, sizeof(path), "%s/PSP/COMMON/GameDiary/db/games.dat", prefix);
    SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd < 0) return -1;

    GameRegistryHeader header;
    sceIoRead(fd, &header, sizeof(GameRegistryHeader));

    g_game_count = header.num_entries;
    g_games = (GameStats*)calloc(g_game_count, sizeof(GameStats));

    for (u32 i = 0; i < g_game_count; i++) {
        sceIoRead(fd, &g_games[i].entry, sizeof(GameEntry));
    }
    sceIoClose(fd);

    /* Build O(1) mapping table for UID -> array index.
     * UIDs are generally sequential, so max_uid is close to g_game_count. */
    if (g_game_count > 0) {
        u32 max_uid = 0;
        for (u32 i = 0; i < g_game_count; i++) {
            if (g_games[i].entry.uid > max_uid) max_uid = g_games[i].entry.uid;
        }
        g_uid_map_size = max_uid + 1;
        g_uid_map = (u16*)malloc(g_uid_map_size * sizeof(u16));
        if (g_uid_map) {
            memset(g_uid_map, 0xFF, g_uid_map_size * sizeof(u16)); /* 0xFFFF means empty */
            for (u32 i = 0; i < g_game_count; i++) {
                g_uid_map[g_games[i].entry.uid] = (u16)i;
            }
        }
    }

    // 2. Load sessions.dat
    snprintf(path, sizeof(path), "%s/PSP/COMMON/GameDiary/db/sessions.dat", prefix);
    fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        SceOff size = sceIoLseek(fd, 0, PSP_SEEK_END);
        g_session_count = size / sizeof(SessionEntry);
        g_sessions = (SessionEntry*)malloc(size);
        sceIoLseek(fd, 0, PSP_SEEK_SET);
        sceIoRead(fd, g_sessions, size);
        sceIoClose(fd);
    }

    return 0;
}

void data_rebuild_uid_map(void) {
    if (!g_uid_map || !g_games) return;
    /* Map each UID to its CURRENT index in the array */
    memset(g_uid_map, 0xFF, g_uid_map_size * sizeof(u16));
    for (u32 i = 0; i < g_game_count; i++) {
        g_uid_map[g_games[i].entry.uid] = (u16)i;
    }
}

void data_calculate_stats(u32 start_ts, u32 end_ts) {
    for (u32 i = 0; i < g_game_count; i++) {
        g_games[i].total_playtime = 0;
        g_games[i].session_count = 0;
        g_games[i].last_played_ts = 0;
        g_games[i].period_playtime = 0;
    }

    for (u32 i = 0; i < g_session_count; i++) {
        u32 uid = g_sessions[i].game_uid;
        if (g_uid_map && uid < g_uid_map_size) {
            u16 idx = g_uid_map[uid];
            if (idx != 0xFFFF && idx < g_game_count) {
                g_games[idx].total_playtime += g_sessions[i].duration;
                g_games[idx].session_count++;
                if (g_sessions[i].timestamp > g_games[idx].last_played_ts) {
                    g_games[idx].last_played_ts = g_sessions[i].timestamp;
                }

                if (g_sessions[i].timestamp >= start_ts && g_sessions[i].timestamp <= end_ts) {
                    g_games[idx].period_playtime += g_sessions[i].duration;
                }
            }
        }
    }
}

u32 data_get_game_count(void) { return g_game_count; }
GameStats* data_get_games(void) { return g_games; }

u32 data_get_session_count(void) { return g_session_count; }
SessionEntry* data_get_sessions(void) { return g_sessions; }

void data_free(void) {
    if (g_games) free(g_games);
    if (g_sessions) free(g_sessions);
    if (g_uid_map) free(g_uid_map);
    g_games = NULL;
    g_sessions = NULL;
    g_uid_map = NULL;
    g_uid_map_size = 0;
}

/**
 * @brief Computes detailed per-period statistics for a single game.
 *
 * Uses utils_get_timestamp() (PSP RTC via sceRtcGetCurrentTick) for all
 * time boundaries instead of time()/mktime(), which may be unreliable on
 * PSP hardware. All timestamp arithmetic is done in u32 UNIX seconds,
 * consistent with SessionEntry.timestamp in the database schema.
 *
 * Period accounting:
 *  - playtime_week  = sessions in the last 7 calendar days
 *  - playtime_month = sessions from start of current month (cumulative, includes week)
 *  - playtime_year  = sessions from start of current year  (cumulative, includes month)
 *
 * @param game_uid  UID of the game to compute stats for.
 * @param out       Output struct; zeroed by this function before filling.
 */
void data_compute_game_details(u32 game_uid, GameDetailStats *out) {
    memset(out, 0, sizeof(*out));

    /* Get current time from the PSP RTC — same approach as compute_weekly_data. */
    u32 now = utils_get_timestamp();
    time_t now_t = (time_t)now;

    /* Strip the time-of-day to get start-of-today. */
    struct tm today_tm = *localtime(&now_t);
    today_tm.tm_hour = 0; today_tm.tm_min = 0; today_tm.tm_sec = 0;
    u32 today_start = (u32)mktime(&today_tm);

    /* Week boundary: 7 days back from today_start (same as weekly graph: 6 days ago). */
    u32 week_start = (today_start > 6 * 86400u) ? (today_start - 6 * 86400u) : 0u;

    /* Month boundary: 1st of current month at 00:00. */
    struct tm month_tm = today_tm;
    month_tm.tm_mday = 1;
    u32 month_start = (u32)mktime(&month_tm);

    /* Year boundary: January 1st of current year at 00:00. */
    struct tm year_tm = today_tm;
    year_tm.tm_mday = 1; year_tm.tm_mon = 0;
    u32 year_start = (u32)mktime(&year_tm);

    /* Accumulate non-overlapping buckets first, then roll up. */
    u32 bucket_week = 0, bucket_month = 0, bucket_year = 0;

    for (u32 i = 0; i < g_session_count; i++) {
        const SessionEntry *s = &g_sessions[i];
        if (s->game_uid != game_uid || s->duration == 0) continue;

        u32 ts = s->timestamp;

        /* Chronological bookmarks. */
        if (out->first_played == 0 || ts < out->first_played) out->first_played = ts;
        if (ts > out->last_played) out->last_played = ts;

        /* Place each session into its tightest non-overlapping bucket. */
        if (ts >= week_start)
            bucket_week += s->duration;
        else if (ts >= month_start)
            bucket_month += s->duration;
        else if (ts >= year_start)
            bucket_year += s->duration;
    }

    /* Roll up: month includes this week; year includes this month (+ week). */
    out->playtime_week  = bucket_week;
    out->playtime_month = bucket_month + bucket_week;
    out->playtime_year  = bucket_year  + bucket_month + bucket_week;
}

