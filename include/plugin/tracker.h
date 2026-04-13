#ifndef _PLUGIN_TRACKER_H_
#define _PLUGIN_TRACKER_H_

/* Needs u32 for the timestamp helper function. */
#include "common/common.h"

/** @brief Starts the background game-time tracking thread. */
void tracker_thread_start(void);

/** @brief Gracefully stops the tracker and flushes the current session. */
void tracker_thread_stop(void);

/** @brief Returns a UNIX timestamp derived from the PSP RTC tick counter. */
u32 get_current_timestamp(void);

#endif /* _PLUGIN_TRACKER_H_ */
