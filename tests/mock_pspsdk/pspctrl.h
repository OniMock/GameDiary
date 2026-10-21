/**
 * Mock pspctrl.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_PSPCTRL_H_
#define _TEST_MOCK_PSPCTRL_H_

#include "psptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum PspCtrlButtons {
    PSP_CTRL_SELECT    = 0x000001,
    PSP_CTRL_START     = 0x000008,
    PSP_CTRL_UP        = 0x000010,
    PSP_CTRL_RIGHT     = 0x000020,
    PSP_CTRL_DOWN      = 0x000040,
    PSP_CTRL_LEFT      = 0x000080,
    PSP_CTRL_LTRIGGER  = 0x000100,
    PSP_CTRL_RTRIGGER  = 0x000200,
    PSP_CTRL_TRIANGLE  = 0x001000,
    PSP_CTRL_CIRCLE    = 0x002000,
    PSP_CTRL_CROSS     = 0x004000,
    PSP_CTRL_SQUARE    = 0x008000,
    PSP_CTRL_HOME      = 0x010000,
    PSP_CTRL_HOLD      = 0x020000,
    PSP_CTRL_NOTE      = 0x800000,
    PSP_CTRL_SCREEN    = 0x400000,
    PSP_CTRL_VOLUP     = 0x100000,
    PSP_CTRL_VOLDOWN   = 0x200000,
};

typedef struct {
    u32 TimeStamp;
    u32 Buttons;
    u8  Lx;
    u8  Ly;
    u8  Rsrv[6];
} SceCtrlData;

int sceCtrlPeekBufferPositive(SceCtrlData *pad_data, int count);
int sceCtrlReadBufferPositive(SceCtrlData *pad_data, int count);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_PSPCTRL_H_ */
