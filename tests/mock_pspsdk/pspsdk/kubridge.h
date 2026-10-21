/**
 * Mock pspsdk/kubridge.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_KUBRIDGE_H_
#define _TEST_MOCK_KUBRIDGE_H_

#include "../psptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int kuKernelInitFileName(char *path);
int kuKernelGetUmdFile(char *path, int size);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_KUBRIDGE_H_ */
