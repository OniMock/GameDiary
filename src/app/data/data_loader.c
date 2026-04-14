#include "app/data/data_loader.h"
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

static GameStats *g_games = NULL;
static u32 g_game_count = 0;
static SessionEntry *g_sessions = NULL;
static u32 g_session_count = 0;

int data_load_all(void) {
    // 1. Load games.dat
    char path[256];
    snprintf(path, sizeof(path), "ms0:/PSP/COMMON/GameDiary/db/games.dat");
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

    // 2. Load sessions.dat
    snprintf(path, sizeof(path), "ms0:/PSP/COMMON/GameDiary/db/sessions.dat");
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

void data_calculate_stats(u32 start_ts, u32 end_ts) {
    for (u32 i = 0; i < g_game_count; i++) {
        g_games[i].total_playtime = 0;
        g_games[i].session_count = 0;
        g_games[i].last_played_ts = 0;
        g_games[i].period_playtime = 0;
    }

    for (u32 i = 0; i < g_session_count; i++) {
        for (u32 j = 0; j < g_game_count; j++) {
            if (g_games[j].entry.uid == g_sessions[i].game_uid) {
                g_games[j].total_playtime += g_sessions[i].duration;
                g_games[j].session_count++;
                if (g_sessions[i].timestamp > g_games[j].last_played_ts) {
                    g_games[j].last_played_ts = g_sessions[i].timestamp;
                }
                
                if (g_sessions[i].timestamp >= start_ts && g_sessions[i].timestamp <= end_ts) {
                    g_games[j].period_playtime += g_sessions[i].duration;
                }
                break;
            }
        }
    }
}

u32 data_get_game_count(void) { return g_game_count; }
GameStats* data_get_games(void) { return g_games; }

void data_free(void) {
    if (g_games) free(g_games);
    if (g_sessions) free(g_sessions);
    g_games = NULL;
    g_sessions = NULL;
}
