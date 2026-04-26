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
#include "common/db_schema.h"
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
    snprintf(path, sizeof(path), "%s" GDIARY_BASE_DIR "/" GDIARY_DB_DIR "/" GAMES_DAT, prefix);
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

    // 2. Load "SESSIONS_DAT"
    snprintf(path, sizeof(path), "%s" GDIARY_BASE_DIR "/" GDIARY_DB_DIR "/" SESSIONS_DAT, prefix);
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
                /* last_played_ts tracks the END of the most recent session
                 * (timestamp + duration) so that cross-midnight sessions
                 * show the day they actually finished on. */
                u32 sess_end_ts = g_sessions[i].timestamp + g_sessions[i].duration;
                if (sess_end_ts > g_games[idx].last_played_ts) {
                    g_games[idx].last_played_ts = sess_end_ts;
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
    g_uid_map_size = 0;}

/**
 * @brief Computes detailed per-period statistics for a single game.
 *
 * Uses utils_get_timestamp() (PSP RTC via sceRtcGetCurrentTick) for all
 * time boundaries. Sessions that cross a calendar boundary are SPLIT so each
 * period only counts the seconds that genuinely fell within its window.
 *
 * Example: a 2-hour session starting at 23:00 on day 25 contributes 1 hour
 * to day 25 and 1 hour to day 26 — not 2 hours to either day alone.
 *
 * Period accounting (all independent, all inclusive of sub-periods):
 *  - playtime_week  = seconds played in the rolling last-7-calendar-days window
 *  - playtime_month = seconds played since the 1st of the current calendar month
 *  - playtime_year  = seconds played since Jan 1st of the current calendar year
 *
 * @param game_uid  UID of the game to compute stats for.
 * @param out       Output struct; zeroed by this function before filling.
 */
void data_compute_game_details(u32 game_uid, GameDetailStats *out) {
    memset(out, 0, sizeof(*out));

    /* Get current time from the PSP RTC. */
    u32 now = utils_get_timestamp();
    time_t now_t = (time_t)now;

    /* Strip the time-of-day to get start-of-today (local midnight). */
    struct tm today_tm = *localtime(&now_t);
    today_tm.tm_hour = 0; today_tm.tm_min = 0; today_tm.tm_sec = 0;
    time_t today_start = mktime(&today_tm);
    time_t today_end   = today_start + 86400; /* exclusive upper bound */

    /* Week: rolling 7-day window (6 days ago 00:00 → end of today). */
    time_t week_start  = (today_start > 6 * 86400) ? (today_start - 6 * 86400) : 0;

    /* Month: 1st of the current calendar month at 00:00. */
    struct tm month_tm = today_tm;
    month_tm.tm_mday = 1;
    time_t month_start = mktime(&month_tm);

    /* Year: January 1st of the current year at 00:00. */
    struct tm year_tm = today_tm;
    year_tm.tm_mday = 1; year_tm.tm_mon = 0;
    time_t year_start = mktime(&year_tm);

    u32 total_week = 0, total_month = 0, total_year = 0;

    for (u32 i = 0; i < g_session_count; i++) {
        const SessionEntry *s = &g_sessions[i];
        if (s->game_uid != game_uid || s->duration == 0) continue;

        /* Chronological bookmarks use the session START timestamp. */
        if (out->first_played == 0 || s->timestamp < out->first_played)
            out->first_played = s->timestamp;
        /* last_played tracks the END of the most recent session so that a
         * session crossing midnight shows the correct next-day date. */
        u32 sess_end = s->timestamp + s->duration;
        if (sess_end > out->last_played)
            out->last_played = sess_end;

        time_t s_start = (time_t)s->timestamp;
        time_t s_end   = s_start + (time_t)s->duration;

        /* Each period window is queried independently via time overlap.
         * Month and year windows both extend to today_end so they include
         * ongoing week activity without double-counting. */
        total_week  += utils_time_overlap_secs(s_start, s_end, week_start,  today_end);
        total_month += utils_time_overlap_secs(s_start, s_end, month_start, today_end);
        total_year  += utils_time_overlap_secs(s_start, s_end, year_start,  today_end);
    }

    /* Each period is independently computed — month includes this week's time. */
    out->playtime_week  = total_week;
    out->playtime_month = total_month;
    out->playtime_year  = total_year;
}



