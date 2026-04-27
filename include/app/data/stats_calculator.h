/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */
#ifndef GAMEDIARY_STATS_CALC_H
#define GAMEDIARY_STATS_CALC_H

#include "app/data/data_loader.h"
#include <time.h>

#define MAX_GRAPH_COLS 31

typedef enum {
    STATS_PERIOD_WEEKLY = 0,
    STATS_PERIOD_MONTHLY,
    STATS_PERIOD_YEARLY,
    STATS_PERIOD_COUNT
} StatsPeriod;

typedef struct {
    StatsPeriod period;
    int offset; // 0 = current, -1 = previous, etc.
} StatsQuery;

typedef struct {
    StatsQuery query;
    u32 column_values[MAX_GRAPH_COLS];
    time_t column_dates[MAX_GRAPH_COLS]; // For exact day references
    int column_count;
    u32 max_value;
    
    // Quick strings for context
    char context_title[128];
    char context_subtitle[128]; 
} StatsGraphData;

/**
 * @brief Populates the graph data struct based on the query.
 * Handles offsets, maximum lengths, and aggregation logic.
 */
void stats_calc_query(const SessionEntry *sessions, int count, StatsQuery query, StatsGraphData *out_data);

/**
 * @brief Sorts the internal game list by period playtime (descending).
 */
void stats_sort_by_period(void);

/**
 * @brief Sorts the internal game list by total playtime (descending).
 */
void stats_sort_by_total(void);

#endif // GAMEDIARY_STATS_CALC_H
