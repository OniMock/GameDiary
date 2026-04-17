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

/**
 * @brief Sorts the internal game list by period playtime (descending).
 */
void stats_sort_by_period(void);

/**
 * @brief Sorts the internal game list by total playtime (descending).
 */
void stats_sort_by_total(void);

#endif // GAMEDIARY_STATS_CALC_H
