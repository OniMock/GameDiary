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
#include <stdlib.h>

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
    }
}

void stats_sort_by_total(void) {
    u32 count = data_get_game_count();
    GameStats* games = data_get_games();
    if (count > 0 && games) {
        qsort(games, count, sizeof(GameStats), compare_total);
    }
}
