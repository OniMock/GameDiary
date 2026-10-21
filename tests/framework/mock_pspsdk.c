/**
 * -------------------------------------------------------------
 *  GameDiary Automated Test Suite
 *  Mock PSP SDK Implementation
 * -------------------------------------------------------------
 */

#define _GNU_SOURCE
#include "mock_pspsdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define MAX_OPEN_FILES 64
#define MAX_PARAMS     16

typedef struct {
    int in_use;
    FILE *fp;
    char path[256];
} MockFile;

typedef struct {
    char key[32];
    u8   data[256];
    u32  len;
    u16  type;
} MockParam;

static char g_sandbox_root[256] = "/tmp/gamediary_sandbox";
static MockFile g_files[MAX_OPEN_FILES];
static MockParam g_params[MAX_PARAMS];
static int g_param_count = 0;

static char g_exec_path[256] = "ms0:/PSP/GAME/GameDiary/EBOOT.PBP";
static char g_umd_path[256] = "";
static int  g_apitype = 0x141; // Homebrew default
static int  g_umd_stat = 0;
static u32  g_hen_version = 0x00010000;
static int  g_devkit_version = 0x06060110; // 6.61
static u32  g_free_mem = 48 * 1024 * 1024; // 48 MB (PSP-2000/3000)
static int  g_sys_lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
static u32  g_buttons = 0;

static u32  g_unix_time = 1776772800U; // Monday 2026-04-20 12:00:00 UTC
static int  g_timezone_offset = 0;     // UTC default
static int  g_vblank_count = 0;

static void *g_mock_fb = (void *)0x04000000;
static int   g_mock_stride = 512;
static int   g_mock_fb_format = PSP_DISPLAY_PIXEL_FORMAT_8888;

/* Fault injection */
static bool g_fail_open = false;
static bool g_fail_write = false;
static bool g_fail_rename = false;
static char g_fail_pattern[64] = "";

/* Ensure directories exist recursively for host path */
static void ensure_parent_dirs(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *slash = strrchr(tmp, '/');
    if (!slash) return;
    *slash = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0777);
            *p = '/';
        }
    }
    mkdir(tmp, 0777);
}

static void translate_path(const char *psp_path, char *out_host_path, size_t max_len) {
    if (!psp_path || psp_path[0] == '\0') {
        snprintf(out_host_path, max_len, "%s/empty", g_sandbox_root);
        return;
    }

    const char *rel = psp_path;
    char prefix[32] = "root";

    const char *colon = strchr(psp_path, ':');
    if (colon) {
        size_t plen = colon - psp_path;
        if (plen < sizeof(prefix)) {
            memcpy(prefix, psp_path, plen);
            prefix[plen] = '\0';
        }
        rel = colon + 1;
        while (*rel == '/') rel++;
    }

    snprintf(out_host_path, max_len, "%s/%s/%s", g_sandbox_root, prefix, rel);
}

void mock_pspsdk_init(const char *sandbox_dir) {
    if (sandbox_dir && sandbox_dir[0] != '\0') {
        strncpy(g_sandbox_root, sandbox_dir, sizeof(g_sandbox_root) - 1);
        g_sandbox_root[sizeof(g_sandbox_root) - 1] = '\0';
    }
    mock_pspsdk_reset();
}

void mock_pspsdk_reset(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (g_files[i].in_use && g_files[i].fp) {
            fclose(g_files[i].fp);
        }
        g_files[i].in_use = 0;
        g_files[i].fp = NULL;
        g_files[i].path[0] = '\0';
    }

    if (g_sandbox_root[0] != '\0') {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" && mkdir -p \"%s\"", g_sandbox_root, g_sandbox_root);
        int ret = system(cmd);
        (void)ret;
    }

    g_param_count = 0;
    g_apitype = 0x141;
    g_umd_stat = 0;
    g_free_mem = 48 * 1024 * 1024;
    g_sys_lang = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
    g_buttons = 0;
    g_unix_time = 1776772800U;
    g_timezone_offset = 0;
    g_vblank_count = 0;

    g_fail_open = false;
    g_fail_write = false;
    g_fail_rename = false;
    g_fail_pattern[0] = '\0';
}

void mock_pspsdk_cleanup(void) {
    mock_pspsdk_reset();
}

void mock_vfs_set_sandbox_root(const char *sandbox_dir) {
    if (sandbox_dir && sandbox_dir[0] != '\0') {
        strncpy(g_sandbox_root, sandbox_dir, sizeof(g_sandbox_root) - 1);
        g_sandbox_root[sizeof(g_sandbox_root) - 1] = '\0';
    }
}

const char *mock_vfs_get_sandbox_root(void) {
    return g_sandbox_root;
}

void mock_vfs_map_file(const char *psp_path, const void *data, size_t size) {
    char host_path[512];
    translate_path(psp_path, host_path, sizeof(host_path));
    ensure_parent_dirs(host_path);

    FILE *fp = fopen(host_path, "wb");
    if (fp) {
        if (data && size > 0) {
            fwrite(data, 1, size, fp);
        }
        fclose(fp);
    }
}

int mock_vfs_file_exists(const char *psp_path) {
    char host_path[512];
    translate_path(psp_path, host_path, sizeof(host_path));
    struct stat st;
    return (stat(host_path, &st) == 0);
}

size_t mock_vfs_read_file(const char *psp_path, void *out_buf, size_t max_size) {
    char host_path[512];
    translate_path(psp_path, host_path, sizeof(host_path));
    FILE *fp = fopen(host_path, "rb");
    if (!fp) return 0;
    size_t rd = fread(out_buf, 1, max_size, fp);
    fclose(fp);
    return rd;
}

void mock_vfs_fail_open(bool enable) { g_fail_open = enable; }
void mock_vfs_fail_write(bool enable) { g_fail_write = enable; }
void mock_vfs_fail_rename(bool enable) { g_fail_rename = enable; }
void mock_vfs_fail_path(const char *pattern) {
    if (pattern) {
        strncpy(g_fail_pattern, pattern, sizeof(g_fail_pattern) - 1);
        g_fail_pattern[sizeof(g_fail_pattern) - 1] = '\0';
    } else {
        g_fail_pattern[0] = '\0';
    }
}

void mock_rtc_set_unix_time(u32 seconds) { g_unix_time = seconds; }
void mock_rtc_advance_seconds(u32 seconds) { g_unix_time += seconds; }
void mock_rtc_set_timezone_offset(int offset_seconds) { g_timezone_offset = offset_seconds; }
u32  mock_rtc_get_unix_time(void) { return g_unix_time; }

void mock_cfw_set_init_param(const char *key, const void *data, u32 len) {
    for (int i = 0; i < g_param_count; i++) {
        if (strcmp(g_params[i].key, key) == 0) {
            u32 copy_len = len < sizeof(g_params[i].data) ? len : sizeof(g_params[i].data);
            memcpy(g_params[i].data, data, copy_len);
            g_params[i].len = copy_len;
            return;
        }
    }
    if (g_param_count < MAX_PARAMS) {
        strncpy(g_params[g_param_count].key, key, sizeof(g_params[g_param_count].key) - 1);
        u32 copy_len = len < sizeof(g_params[g_param_count].data) ? len : sizeof(g_params[g_param_count].data);
        memcpy(g_params[g_param_count].data, data, copy_len);
        g_params[g_param_count].len = copy_len;
        g_params[g_param_count].type = 0;
        g_param_count++;
    }
}

void mock_cfw_clear_init_params(void) { g_param_count = 0; }
void mock_cfw_set_executable_path(const char *path) {
    if (path) {
        strncpy(g_exec_path, path, sizeof(g_exec_path) - 1);
        g_exec_path[sizeof(g_exec_path) - 1] = '\0';
    }
}
void mock_cfw_set_apitype(int apitype) { g_apitype = apitype; }
void mock_cfw_set_umd_file(const char *iso_path) {
    if (iso_path) {
        strncpy(g_umd_path, iso_path, sizeof(g_umd_path) - 1);
        g_umd_path[sizeof(g_umd_path) - 1] = '\0';
    } else {
        g_umd_path[0] = '\0';
    }
}
void mock_cfw_set_umd_stat(int stat) { g_umd_stat = stat; }
void mock_cfw_set_hen_version(u32 version) { g_hen_version = version; }
void mock_cfw_set_devkit_version(int version) { g_devkit_version = version; }

void mock_hardware_set_total_free_mem(u32 bytes) { g_free_mem = bytes; }
void mock_hardware_set_system_language(int lang) { g_sys_lang = lang; }
void mock_ctrl_set_buttons(u32 buttons) { g_buttons = buttons; }
u32  mock_ctrl_get_buttons(void) { return g_buttons; }

void mock_display_set_active_fb(void *vram, int stride, int format) {
    g_mock_fb = vram;
    g_mock_stride = stride;
    g_mock_fb_format = format;
}

int mock_display_get_vblank_count(void) { return g_vblank_count; }

/* -------------------------------------------------------------
 * PSP SDK API Implementations
 * ------------------------------------------------------------- */

SceUID sceIoOpen(const char *file, int flags, int mode) {
    (void)mode;
    if (g_fail_open) return -1;
    if (g_fail_pattern[0] != '\0' && strstr(file, g_fail_pattern) != NULL) {
        return -2;
    }

    char host_path[512];
    translate_path(file, host_path, sizeof(host_path));

    int access_mode = flags & 0x0003;
    const char *fmode = "rb";
    if (access_mode == PSP_O_WRONLY || access_mode == PSP_O_RDWR) {
        ensure_parent_dirs(host_path);
        if (flags & PSP_O_TRUNC) {
            fmode = (access_mode == PSP_O_RDWR) ? "w+b" : "wb";
        } else if (flags & PSP_O_APPEND) {
            fmode = (access_mode == PSP_O_RDWR) ? "a+b" : "ab";
        } else {
            fmode = (flags & PSP_O_CREAT) ? "w+b" : "r+b";
        }
    }

    FILE *fp = fopen(host_path, fmode);
    if (!fp && (access_mode == PSP_O_RDWR || access_mode == PSP_O_WRONLY) && (flags & PSP_O_CREAT)) {
        fp = fopen(host_path, "w+b");
    }
    if (!fp) return -3;

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!g_files[i].in_use) {
            g_files[i].in_use = 1;
            g_files[i].fp = fp;
            strncpy(g_files[i].path, host_path, sizeof(g_files[i].path) - 1);
            return (SceUID)(i + 10);
        }
    }

    fclose(fp);
    return -4; // No file slots
}

int sceIoClose(SceUID fd) {
    int idx = (int)fd - 10;
    if (idx < 0 || idx >= MAX_OPEN_FILES || !g_files[idx].in_use) return -1;

    if (g_files[idx].fp) {
        fclose(g_files[idx].fp);
        g_files[idx].fp = NULL;
    }
    g_files[idx].in_use = 0;
    g_files[idx].path[0] = '\0';
    return 0;
}

int sceIoRead(SceUID fd, void *data, SceSize size) {
    int idx = (int)fd - 10;
    if (idx < 0 || idx >= MAX_OPEN_FILES || !g_files[idx].in_use || !g_files[idx].fp) return -1;
    size_t rd = fread(data, 1, size, g_files[idx].fp);
    return (int)rd;
}

int sceIoWrite(SceUID fd, const void *data, SceSize size) {
    if (g_fail_write) return -1;
    int idx = (int)fd - 10;
    if (idx < 0 || idx >= MAX_OPEN_FILES || !g_files[idx].in_use || !g_files[idx].fp) return -1;
    size_t wr = fwrite(data, 1, size, g_files[idx].fp);
    fflush(g_files[idx].fp);
    return (int)wr;
}

SceOff sceIoLseek(SceUID fd, SceOff offset, int whence) {
    int idx = (int)fd - 10;
    if (idx < 0 || idx >= MAX_OPEN_FILES || !g_files[idx].in_use || !g_files[idx].fp) return -1;

    int posix_whence = SEEK_SET;
    if (whence == PSP_SEEK_CUR) posix_whence = SEEK_CUR;
    else if (whence == PSP_SEEK_END) posix_whence = SEEK_END;

    if (fseeko(g_files[idx].fp, (off_t)offset, posix_whence) != 0) return -2;
    return (SceOff)ftello(g_files[idx].fp);
}

int sceIoRemove(const char *file) {
    char host_path[512];
    translate_path(file, host_path, sizeof(host_path));
    return unlink(host_path) == 0 ? 0 : -1;
}

int sceIoRename(const char *oldname, const char *newname) {
    if (g_fail_rename) return -1;
    char host_old[512], host_new[512];
    translate_path(oldname, host_old, sizeof(host_old));
    translate_path(newname, host_new, sizeof(host_new));
    ensure_parent_dirs(host_new);
    return rename(host_old, host_new) == 0 ? 0 : -2;
}

int sceIoMkdir(const char *dir, int mode) {
    (void)mode;
    char host_path[512];
    translate_path(dir, host_path, sizeof(host_path));
    ensure_parent_dirs(host_path);
    return mkdir(host_path, 0777) == 0 || errno == EEXIST ? 0 : -1;
}

int sceIoRmdir(const char *dir) {
    char host_path[512];
    translate_path(dir, host_path, sizeof(host_path));
    return rmdir(host_path) == 0 ? 0 : -1;
}

int sceIoSync(const char *device, unsigned int flag) {
    (void)device;
    (void)flag;
    return 0;
}

SceUID sceIoDopen(const char *dirname) {
    char host_path[512];
    translate_path(dirname, host_path, sizeof(host_path));
    struct stat st;
    if (stat(host_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 999;
    }
    return -1;
}

int sceIoDclose(SceUID dir) {
    (void)dir;
    return 0;
}

void sceKernelDelayThread(unsigned int microseconds) {
    (void)microseconds;
    /* Do not block tests */
}

int sceKernelDevkitVersion(void) {
    return g_devkit_version;
}

u32 sceKernelTotalFreeMemSize(void) {
    return g_free_mem;
}

int sceKernelInitApitype(void) {
    return g_apitype;
}

void sceKernelExitGame(void) {
}

int sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *common) {
    (void)name; (void)func; (void)common;
    return 1;
}

int sceKernelRegisterExitCallback(int cbid) {
    (void)cbid;
    return 0;
}

int sceKernelSleepThreadCB(void) {
    return 0;
}

SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority,
                             int stackSize, int attr, void *option) {
    (void)name; (void)entry; (void)initPriority; (void)stackSize; (void)attr; (void)option;
    return 200;
}

int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp) {
    (void)thid; (void)arglen; (void)argp;
    return 0;
}

int sceKernelWaitThreadEnd(SceUID thid, void *timeout) {
    (void)thid; (void)timeout;
    return 0;
}

int sceKernelDeleteThread(SceUID thid) {
    (void)thid;
    return 0;
}

SceUID sceKernelCreateSema(const char *name, int attr, int initVal, int maxVal, void *opt) {
    (void)name; (void)attr; (void)initVal; (void)maxVal; (void)opt;
    return 300;
}

int sceKernelWaitSema(SceUID semaid, int signal, void *timeout) {
    (void)semaid; (void)signal; (void)timeout;
    return 0;
}

int sceKernelSignalSema(SceUID semaid, int signal) {
    (void)semaid; (void)signal;
    return 0;
}

int sceKernelDeleteSema(SceUID semaid) {
    (void)semaid;
    return 0;
}

int sceKernelCreateLwMutex(SceLwMutexWorkarea *workarea, const char *name, int attr, int initCount, void *opt) {
    (void)name; (void)attr; (void)initCount; (void)opt;
    if (workarea) {
        memset(workarea, 0, sizeof(*workarea));
    }
    return 0;
}

int sceKernelLockLwMutex(SceLwMutexWorkarea *workarea, int count, void *timeout) {
    (void)workarea; (void)count; (void)timeout;
    return 0;
}

int sceKernelUnlockLwMutex(SceLwMutexWorkarea *workarea, int count) {
    (void)workarea; (void)count;
    return 0;
}

int sceKernelDeleteLwMutex(SceLwMutexWorkarea *workarea) {
    (void)workarea;
    return 0;
}

int sceRtcGetCurrentTick(u64 *tick) {
    if (!tick) return -1;
    // tick is microseconds since 0001-01-01. Difference with UNIX epoch is 62135596800 seconds.
    *tick = ((u64)g_unix_time + 62135596800ULL) * 1000000ULL;
    return 0;
}

int sceRtcGetCurrentClock(ScePspDateTime *time, int tz) {
    (void)tz;
    if (!time) return -1;
    time_t t = (time_t)g_unix_time;
    struct tm tm = *gmtime(&t);
    time->year = tm.tm_year + 1900;
    time->month = tm.tm_mon + 1;
    time->day = tm.tm_mday;
    time->hour = tm.tm_hour;
    time->minute = tm.tm_min;
    time->second = tm.tm_sec;
    time->microsecond = 0;
    return 0;
}

int sceRtcConvertUtcToLocalTime(const u64 *utc, u64 *local) {
    if (!utc || !local) return -1;
    *local = (u64)((s64)*utc + ((s64)g_timezone_offset * 1000000LL));
    return 0;
}

u32 sceRtcGetTickResolution(void) {
    return 1000000;
}

int sceCtrlPeekBufferPositive(SceCtrlData *pad_data, int count) {
    if (!pad_data || count <= 0) return 0;
    pad_data->Buttons = g_buttons;
    pad_data->TimeStamp = g_unix_time * 1000;
    pad_data->Lx = 128;
    pad_data->Ly = 128;
    return count;
}

int sceCtrlReadBufferPositive(SceCtrlData *pad_data, int count) {
    return sceCtrlPeekBufferPositive(pad_data, count);
}

int sceDisplayGetFrameBuf(void **topaddr, int *bufferwidth, int *pixelformat, int sync) {
    (void)sync;
    if (topaddr) *topaddr = g_mock_fb;
    if (bufferwidth) *bufferwidth = g_mock_stride;
    if (pixelformat) *pixelformat = g_mock_fb_format;
    return 0;
}

int sceDisplaySetFrameBuf(void *topaddr, int bufferwidth, int pixelformat, int sync) {
    (void)sync;
    g_mock_fb = topaddr;
    g_mock_stride = bufferwidth;
    g_mock_fb_format = pixelformat;
    return 0;
}

int sceDisplayGetMode(int *mode, int *width, int *height) {
    if (mode) *mode = 0;
    if (width) *width = 480;
    if (height) *height = 272;
    return 0;
}

int sceDisplayWaitVblankStart(void) {
    g_vblank_count++;
    return 0;
}

int scePowerGetBatteryLifePercent(void) { return 85; }
int scePowerIsBatteryCharging(void) { return 0; }

void sceGuColor(u32 color) { (void)color; }

int sceUtilityGetSystemParamInt(int id, int *val) {
    if (id == PSP_SYSTEMPARAM_ID_INT_LANGUAGE && val) {
        if (g_sys_lang < 0) return -1;
        *val = g_sys_lang;
        return 0;
    }
    return -1;
}

int sctrlGetInitPARAM(const char *key, u16 *type, u32 *len, void *val) {
    if (!key || !len || !val) return -1;
    for (int i = 0; i < g_param_count; i++) {
        if (strcmp(g_params[i].key, key) == 0) {
            u32 copy_len = g_params[i].len;
            memcpy(val, g_params[i].data, copy_len);
            *len = g_params[i].len;
            if (type) *type = g_params[i].type;
            return 0;
        }
    }
    return -2;
}

u32 sctrlHENGetVersion(void) {
    return g_hen_version;
}

int sceUmdGetDriveStat(void) {
    return g_umd_stat;
}

static int mock_get_drive_stat_fn(void) {
    return g_umd_stat;
}

u32 sctrlHENFindFunction(const char *szMod, const char *szLib, u32 nid) {
    (void)szMod; (void)szLib;
    if (nid == 0x6B4A146C) { // SCE_UMD_GET_DRIVE_STAT_NID
        return (u32)(uintptr_t)mock_get_drive_stat_fn;
    }
    return 0;
}

int sctrlHENPatchSyscall(u32 addr, void *newfunc) {
    (void)addr; (void)newfunc;
    return 0;
}

int kuKernelInitFileName(char *path) {
    if (!path) return -1;
    strncpy(path, g_exec_path, 255);
    path[255] = '\0';
    return 0;
}

int kuKernelGetUmdFile(char *path, int size) {
    if (!path || size <= 0) return -1;
    strncpy(path, g_umd_path, size - 1);
    path[size - 1] = '\0';
    return g_umd_path[0] != '\0' ? 0 : -1;
}
