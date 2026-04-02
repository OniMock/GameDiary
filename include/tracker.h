#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "common.h"

// Start the background tracking thread
void tracker_thread_start(void);

// Stop the tracker gracefully
void tracker_thread_stop(void);

// Helper to get UNIX timestamp using PSP RTC
u32 get_current_timestamp(void);

#endif
