/**
 * Mock pspkernel.h for GameDiary test suite
 */
#ifndef _TEST_MOCK_PSPKERNEL_H_
#define _TEST_MOCK_PSPKERNEL_H_

#include "psptypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Module info macro */
#define PSP_MODULE_INFO(name, attr, major, minor)
#define PSP_MAIN_THREAD_ATTR(attr)
#define PSP_MAIN_THREAD_PRIORITY(prio)
#define PSP_MAIN_THREAD_STACK_SIZE_KB(kb)

#define PSP_THREAD_ATTR_USER 0

/* File I/O flags */
#define PSP_O_RDONLY   0x0001
#define PSP_O_WRONLY   0x0002
#define PSP_O_RDWR     0x0003
#define PSP_O_NBLOCK   0x0004
#define PSP_O_DIROPEN  0x0008
#define PSP_O_APPEND   0x0100
#define PSP_O_CREAT    0x0200
#define PSP_O_TRUNC    0x0400
#define PSP_O_EXCL     0x0800
#define PSP_O_NOWAIT   0x8000

#define PSP_SEEK_SET   0
#define PSP_SEEK_CUR   1
#define PSP_SEEK_END   2

/* Mock File I/O functions */
SceUID sceIoOpen(const char *file, int flags, int mode);
int    sceIoClose(SceUID fd);
int    sceIoRead(SceUID fd, void *data, SceSize size);
int    sceIoWrite(SceUID fd, const void *data, SceSize size);
SceOff sceIoLseek(SceUID fd, SceOff offset, int whence);
int    sceIoRemove(const char *file);
int    sceIoRename(const char *oldname, const char *newname);
int    sceIoMkdir(const char *dir, int mode);
int    sceIoRmdir(const char *dir);
int    sceIoSync(const char *device, unsigned int flag);
SceUID sceIoDopen(const char *dirname);
int    sceIoDclose(SceUID dir);

/* Mock Kernel Utilities */
void   sceKernelDelayThread(unsigned int microseconds);
int    sceKernelDevkitVersion(void);
u32    sceKernelTotalFreeMemSize(void);
int    sceKernelInitApitype(void);
void   sceKernelExitGame(void);

/* Mock Callbacks & Threads */
typedef int (*SceKernelCallbackFunction)(int arg1, int arg2, void *arg);
typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);

int    sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *common);
int    sceKernelRegisterExitCallback(int cbid);
int    sceKernelSleepThreadCB(void);

SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority,
                             int stackSize, int attr, void *option);
int    sceKernelStartThread(SceUID thid, SceSize arglen, void *argp);
int    sceKernelWaitThreadEnd(SceUID thid, void *timeout);
int    sceKernelDeleteThread(SceUID thid);

/* Mock Semaphores */
SceUID sceKernelCreateSema(const char *name, int attr, int initVal, int maxVal, void *opt);
int    sceKernelWaitSema(SceUID semaid, int signal, void *timeout);
int    sceKernelSignalSema(SceUID semaid, int signal);
int    sceKernelDeleteSema(SceUID semaid);

/* Mock Lightweight Mutex */
typedef struct {
    int lock_count;
    void *owner;
    int pad[6];
} SceLwMutexWorkarea;

int    sceKernelCreateLwMutex(SceLwMutexWorkarea *workarea, const char *name, int attr, int initCount, void *opt);
int    sceKernelLockLwMutex(SceLwMutexWorkarea *workarea, int count, void *timeout);
int    sceKernelUnlockLwMutex(SceLwMutexWorkarea *workarea, int count);
int    sceKernelDeleteLwMutex(SceLwMutexWorkarea *workarea);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_MOCK_PSPKERNEL_H_ */
