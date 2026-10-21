/**
 * Mock psprtc.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_PSPRTC_H_
#define _TEST_MOCK_PSPRTC_H_

#include "psptypes.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u16 year;
    u16 month;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u32 microsecond;
} ScePspDateTime;

int sceRtcGetCurrentTick(u64 *tick);
int sceRtcGetCurrentClock(ScePspDateTime *time, int tz);
int sceRtcConvertUtcToLocalTime(const u64 *utc, u64 *local);
u32 sceRtcGetTickResolution(void);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_PSPRTC_H_ */
