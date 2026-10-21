/**
 * Mock pspdisplay.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_PSPDISPLAY_H_
#define _TEST_MOCK_PSPDISPLAY_H_

#include "psptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_DISPLAY_SETBUF_IMMEDIATE 0
#define PSP_DISPLAY_SETBUF_NEXTFRAME 1

enum PspDisplayPixelFormats {
    PSP_DISPLAY_PIXEL_FORMAT_565 = 0,
    PSP_DISPLAY_PIXEL_FORMAT_5551,
    PSP_DISPLAY_PIXEL_FORMAT_4444,
    PSP_DISPLAY_PIXEL_FORMAT_8888
};

int sceDisplayGetFrameBuf(void **topaddr, int *bufferwidth, int *pixelformat, int sync);
int sceDisplaySetFrameBuf(void *topaddr, int bufferwidth, int pixelformat, int sync);
int sceDisplayGetMode(int *mode, int *width, int *height);
int sceDisplayWaitVblankStart(void);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_PSPDISPLAY_H_ */
