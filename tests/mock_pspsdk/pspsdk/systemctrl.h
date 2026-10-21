/**
 * Mock pspsdk/systemctrl.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_SYSTEMCTRL_H_
#define _TEST_MOCK_SYSTEMCTRL_H_

#include "../psptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 addr;
    u32 data;
} FunctionPatchData;

int sctrlGetInitPARAM(const char *key, u16 *type, u32 *len, void *val);
u32 sctrlHENFindFunction(const char *szMod, const char *szLib, u32 nid);
u32 sctrlHENGetVersion(void);
int sctrlHENPatchSyscall(u32 addr, void *newfunc);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_SYSTEMCTRL_H_ */
