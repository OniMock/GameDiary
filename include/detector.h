#ifndef _DETECTOR_H_
#define _DETECTOR_H_

#include "common.h"

// Parse SFO and initialize game metadata immediately
void detector_init(void);

// Fetch Game ID / Title from disc after the system has stabilized
void detector_init_late(void);

#endif
