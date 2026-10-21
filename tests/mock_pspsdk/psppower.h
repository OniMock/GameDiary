/**
 * Mock psppower.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_PSPPOWER_H_
#define _TEST_MOCK_PSPPOWER_H_

#include "psptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int scePowerGetBatteryLifePercent(void);
int scePowerIsBatteryCharging(void);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_PSPPOWER_H_ */
