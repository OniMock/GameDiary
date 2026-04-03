#ifndef _DETECTOR_H_
#define _DETECTOR_H_

#include "models.h"

// Parse SFO and initialize game metadata immediately
void detector_init(void);

// Fetch Game ID / Title from disc after the system has stabilized
void detector_init_late(void);

/**
 * @brief Gets a pointer to the current game metadata.
 *
 * @return const GameMetadata*
 */
const GameMetadata *detector_get_metadata(void);

#endif
