/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 */

#ifndef _SYSTEMCTRL_H_
#define _SYSTEMCTRL_H_

#include <pspinit.h>
#include <pspiofilemgr_kernel.h>
#include <pspkernel.h>
#include <psploadcore.h>
#include <psploadexec_kernel.h>
#include <pspsdk.h>
#include <pspthreadman_kernel.h>

#include <rebootexconfig.h>

// game id is 9 bytes long
#define GAME_ID_MINIMUM_BUFFER_SIZE 10

// Not much of a difference when past 4Kb
#define MSCACHE_BUFSIZE_MIN (4 * 1024)
#define MSCACHE_BUFSIZE_MED (8 * 1024)
#define MSCACHE_BUFSIZE_MAX (16 * 1024)

// Different PSP models
enum {
  PSP_1000 = 0,   // 01g
  PSP_2000 = 1,   // 02g
  PSP_3000 = 2,   // 03g
  PSP_4000 = 3,   // 04g
  PSP_GO = 4,     // 05g
  PSP_7000 = 6,   // 07g
  PSP_9000 = 8,   // 09g
  PSP_11000 = 10, // 11g
};

enum BootLoadFlags {
  BOOTLOAD_VSH = 1,
  BOOTLOAD_GAME = 2,
  BOOTLOAD_UPDATER = 4,
  BOOTLOAD_POPS = 8,
  BOOTLOAD_UMDEMU = 64, /* for original NP9660 */
};

// Console type (value returned by sctrlHENIsToolKit)
enum ToolkitType {
  PSP_TOOLKIT_TYPE_RETAIL = 0, // retail unit (not a toolkit)
  PSP_TOOLKIT_TYPE_TEST = 1,   // testing tool kit
  PSP_TOOLKIT_TYPE_DEV = 2,    // development tool kit
};

// Custom Utility Modules
#define PSP_MODULE_NET_FTP 0x0180
#define PSP_MODULE_AV_PNG 0x0380
#define PSP_MODULE_AV_HELPER 0x0390
#define PSP_MODULE_VLF 0x0700
#define PSP_MODULE_INTRAFONT_VLF 0x0701
#define PSP_MODULE_INTRAFONT_GU 0x0702
#define PSP_MODULE_UNARCHIVER 0x0710
#define PSP_MODULE_USB_DEV_DRV 0x0800
#define PSP_MODULE_IDS_REGEN 0x0801
#define PSP_MODULE_IO_PRIVILEGED 0x0802
#define PSP_MODULE_PSAR_DUMPER 0x0803
#define PSP_MODULE_PSPDECRYPT 0x0804
#define PSP_MODULE_KPSPIDENT 0x0805
#define PSP_MODULE_IPL_UPDATER 0x0806
#define PSP_MODULE_KBOOTI_UPDATER 0x0807

// Prologue Module Start Handler
typedef int (*STMOD_HANDLER)(SceModule *);

// data for HijackFunction
typedef struct FunctionPatchData {
  unsigned int __instr__[5];
  unsigned int __extra_[3];
} FunctionPatchData;

// Decrypt extension functions
typedef int (*KDEC_HANDLER)(u32 *buf, int size, int *retSize, int m);
typedef int (*MDEC_HANDLER)(u32 *tag, u8 *keys, u32 code, u32 *buf, int size,
                            int *retSize, int m, void *unk0, int unk1, int unk2,
                            int unk3, int unk4);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// patch helper functions
u32 sctrlHENFindJALGeneric(u32 addr, int reversed, int skip);
#define sctrlHENFindFirstJAL(addr) sctrlHENFindJALGeneric(addr, 0, 0)
#define sctrlHENFindFirstJALReverse(addr) sctrlHENFindJALGeneric(addr, 1, 0)
#define sctrlHENFindJAL(addr, pos) sctrlHENFindJALGeneric(addr, 0, pos)
#define sctrlHENFindJALReverse(addr, pos) sctrlHENFindJALGeneric(addr, 1, pos)
#define sctrlHENFindFirstJALForFunction(modname, libname, uid)                 \
  sctrlHENFindFirstJAL(sctrlHENFindFunction(modname, libname, uid))
#define sctrlHENFindJALForFunction(modname, libname, uid, pos)                 \
  sctrlHENFindJAL(sctrlHENFindFunction(modname, libname, uid), pos)
#define sctrlHENFindFirstJALReverseForFunction(modname, libname, uid)          \
  sctrlHENFindFirstJALReverse(sctrlHENFindFunction(modname, libname, uid))
#define sctrlHENFindJALReverseForFunction(modname, libname, uid, pos)          \
  sctrlHENFindJALReverse(sctrlHENFindFunction(modname, libname, uid), pos)

u32 sctrlHENFindFirstBEQ(u32 addr);
u32 sctrlHENFindRefInGlobals(char *libname, u32 addr, u32 ptr);

/**
 * Checks if the system already started.
 *
 * @return `1` if system has started, `0` otherwise.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENIsSystemBooted(void);

/**
 *  Find a import library stub table.
 *
 * @param[in] mod The module where to search the function.
 * @param[in] library The library name.
 *
 * @return The reference to the stub table, or `NULL` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
SceLibraryStubTable *sctrlFindImportLib(SceModule *mod, const char *library);

/**
 *  Find Import Function Stub Address
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlFindImportByNID(SceModule *pMod, const char *library, u32 nid);

/**
 * Replace import function stub in a module with a function or dummy value.
 *
 * This function autodetects whether Syscalls are used or not. If syscall is
 * detected, the function creates a new syscall and redirects the stub to the
 * `func` syscall. Manually exporting in `exports.exp` is still required for
 * Syscalls to work.
 *
 * @param[in] mod The module where to search the function
 * @param[in] library The library name
 * @param[in] nid The nid of the function
 * @param[in] func The function to replace the stub. If value is below 16bit max
 * value, this is interpreted as a dummy value
 *
 * @return
 * `0` if successful;
 * `-1` if `mod` or `library` are `NULL`;
 * `-2` if failed to find import by NID and fail to resolve that NID from older
 * firmware version;
 * `-3` if failed to find import by NID after successful resolve to older
 * firmware version;
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHookImportByNID(SceModule *mod, const char *library, u32 nid,
                         void *func);

/**
 * Calculate Random Number via KIRK.
 *
 * @return A random number.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlKernelRand(void);

/**
 * Set custom start module handler.
 *
 * This function register custom `init.prx` `sceKernelStartModule` handler.
 *
 * It can be used to replace a system module.
 *
 * @param[in] func A function pointer to register as custom start module
 * handler.
 *
 * @return The previously set module handler, if any.
 *
 * @note The `func` returns `-1` to ignore the module and load the original
 * module. Or new `modid` if a replace is done.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
 */
void *sctrlSetStartModuleExtra(int (*func)(int modid, SceSize argsize,
                                           void *argp, int *modstatus,
                                           SceKernelSMOption *opt));

/**
 * Read parameter from an SFO file or an EBOOT.PBP file.
 *
 * @param[in] sfo_path The SFO file path, or `NULL` for EBOOT.PBP file.
 * @param[in] param_name The SFO parameter name.
 * @param[out] param_type A pointer to receive the SFO parameter type. It can be
 * `NULL` to not receive it.
 * @param[out] param_length A pointer to receive the SFO parameter length. It
 * can be `NULL` to not receive it.
 * @param[out] param_buf The buffer to write the the found SFO parameter. It can
 * be `NULL` to not receive it.
 *
 * @return Returns `0` if parameter was found, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlGetSfoPARAM(const char *sfo_path, const char *paramName,
                     u16 *paramType, u32 *paramLength, void *paramBuffer);

/**
 * Get SFO param from currently running game/app.
 *
 * @param[in] param_name The SFO parameter name.
 * @param[out] param_type A pointer to receive the SFO parameter type. It can be
 * `NULL` to not receive it.
 * @param[out] param_length A pointer to receive the SFO parameter length. It
 * can be `NULL` to not receive it.
 * @param[out] param_buf The buffer to write the the found SFO parameter. It can
 * be `NULL` to not receive it.
 *
 * @return Returns `0` if parameter was found, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlGetInitPARAM(const char *paramName, u16 *paramType, u32 *paramLength,
                      void *paramBuffer);

/**
 * Finds the UID of the thread with a given `name`.
 *
 * @param name The name of the thread.
 *
 * @return The UID of the thread on success, `< 0` on error or not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlGetThreadUIDByName(const char *name);

/**
 * Return Thread Context of specified Thread (Search by UID)
 */
int sctrlGetThreadContextByUID(int uid, SceKernelThreadKInfo *ctx);

/**
 * Return Thread Context of specified Thread (Search by Name)
 */
int sctrlGetThreadContextByName(const char *name, SceKernelThreadKInfo *ctx);

/**
 * Flush/Cleans instruction and data caches.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
void sctrlFlushCache(void);

/**
 * Register the default VRAM handler for PSX exploit, returns the previous
 * handler
 */
void *sctrlHENSetPSXVramHandler(void (*handler)(u32 *psp_vram, u16 *ps1_vram));

/**
 * Decompress inflate data.
 *
 * This function is a wrap for `sceKernelDeflateDecompress`.
 *
 * @param[out] dest A pointer to destination buffer.
 * @param[out] dest_size The size of destination buffer.
 * @param[in] src A pointer to source (compressed) data.
 *
 * @return The decompressed size on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` stub to be available.
 */
int sctrlDeflateDecompress(void *dest, void *src, int size);

/**
 * Decompress GZIP-compressed data.
 *
 * @param[out] dest A pointer to destination buffer.
 * @param[in] src A pointer to source (compressed) data.
 * @param size The size of destination buffer.
 *
 * @return The decompressed size on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` stub to be available.
 */
int sctrlGzipDecompress(void *dest, void *src, int size);

/**
 * Decompress LZ4-compressed data.
 *
 * @param[in] source A pointer to source (compressed) data.
 * @param[out] dest A pointer to destination buffer.
 * @param outputSize The size of destination buffer.
 *
 * @return The number of bytes read from the source buffer on success, `< 0` on
 * error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int LZ4_decompress_fast(const char *source, char *dest, int outputSize);

/**
 * Decompress LZ4-compressed data.
 *
 * @param[out] dest A pointer to destination buffer.
 * @param[in] src A pointer to source (compressed) data.
 * @param size The size of destination buffer.
 *
 * @return The number of bytes read from the source buffer on success, `< 0` on
 * error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
#define sctrlLz4Decompress(dest, src, size) LZ4_decompress_fast(src, dest, size)

/**
 * Decompress LZ4-compressed data.
 *
 * @param[in] source A pointer to source (compressed) data.
 * @param src_len The size of the compressed data
 * @param[out] dest A pointer to destination buffer.
 * @param[out] dst_len A pointer to receive the destination buffer size.
 * @param unused Unused. Pass `NULL`.
 *
 * @return `0` on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int lzo1x_decompress(void *source, unsigned src_len, void *dest,
                     unsigned *dst_len, void *unused);

/**
 * Decompress LZ4-compressed data.
 *
 * @param[in] source A pointer to source (compressed) data.
 * @param src_len The size of the compressed data
 * @param[out] dest A pointer to destination buffer.
 * @param[out] dst_len A pointer to receive the destination buffer size.
 *
 * @return `0` on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
#define sctrlLzoDecompress(dest, dst_size, src, src_size)                      \
  lzo1x_decompress(src, src_size, dest, dst_size, NULL)

/**
 * Check if currently running with ms0 being redirected to ef0.
 */
int sctrlKernelMsIsEf();

/**
 * Restart the vsh.
 *
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelExitVSH(struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a disc.
 *
 * It is the function used by the firmware to execute the EBOOT.BIN from a disc.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHDisc(const char *file,
                               struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a disc.
 *
 * It is the function used by the firmware to execute an updater from a disc.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHDiscUpdater(const char *file,
                                      struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a memory stick.
 *
 * It is the function used by the firmware to execute an updater from a memory
 * stick.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHMs1(const char *file,
                              struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a memory stick.
 *
 * It is the function used by the firmware to execute games (and homebrew :P)
 * from a memory stick.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHMs2(const char *file,
                              struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from the internal memory.
 *
 * It is the function used by the firmware to execute games (and homebrew :P)
 * from the internal memory.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHEf2(const char *file,
                              struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a memory stick.
 *
 * It is the function used by the firmware to execute ... ?
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHMs3(const char *file,
                              struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable from a memory stick.
 *
 * It is the function used by the firmware to execute application (e.g. Comic
 * Reader) from a memory stick.
 *
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHMs4(const char *file,
                              struct SceKernelLoadExecVSHParam *param);

/**
 * Executes a new executable with the specified apitype
 *
 * @param apitype The apitype
 * @param[in] file The file to execute.
 * @param[in] param A pointer to a `SceKernelLoadExecVSHParam` structure, or
 * `NULL`.
 *
 * @return `< 0` on some errors.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelLoadExecVSHWithApitype(int apitype, const char *file,
                                      struct SceKernelLoadExecVSHParam *param);

/**
 * Sets the API type.
 *
 * @param apitype The apitype to set. One of `PSPInitApitype`
 *
 * @return The previous apitype
 *
 * @note This will modify also the value of `SceBootMediumType`, since the value
 * of bootfrom is calculated from the apitype
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetInitApitype(int apitype);

/**
 * Sets the filename of the launched executable.
 *
 * @param[in] filename The filename to set.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetInitFileName(char *filename);

/**
 * Sets the application type.
 *
 * @param key The key code. One of `SceApplicationType`.
 *
 * @return the previous Application type.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetApplicationType(int key);

/**
 * Sets the init key config. (Same as `sctrlKernelSetApplicationType()`).
 *
 * @param key The key code. One of `SceApplicationType`.
 *
 * @return the previous Application type.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetInitKeyConfig(int key);

/**
 * Sets the user level of the current thread.
 *
 * @param level The user level.
 *
 * @return The previous user level on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetUserLevel(int level);

/**
 * Sets the devkit version.
 *
 * @param version The devkit version to set.
 *
 * @return The previous devkit version.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelSetDevkitVersion(int version);

/**
 * Checks if we are in SE.
 *
 * @return 1 if we are in SE-C or later, 0 if we are in HEN-D or later,
 * and `< 0` (a kernel error code) in any other case.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENIsSE();

/**
 * Checks if we are in Devhook.
 *
 * @return `1` if we are in SE-C/HEN-D for devhook or later, `0` if we are in
 * normal SE-C/HEN-D or later, and `< 0` (a kernel error code) in any other
 * case.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENIsDevhook();

/**
 * Gets the HEN version.
 *
 * @return The HEN version.
 *
 * HEN D / SE-C :  0x00000400
 * M33 : 0x00000700 | 0x00000800
 * TN / Adrenaline : 0x00001000
 * PRO : 0x00001003
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENGetVersion();

/**
 * Gets the HEN minor version
 *
 * @returns - The HEN minor version
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENGetMinorVersion();

/**
 * Check if currently running on a toolkit.
 *
 * @return `0` if retail, `1` if TestingTool, `2` if DevelopmentTool.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENIsToolKit();

/**
 * Finds a driver.
 *
 * @param[in] drvname The name of the driver (without ":" or numbers).
 *
 * @return The driver if found, `NULL` otherwise.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
PspIoDrv *sctrlHENFindDriver(const char *drvname);

/**
 * Finds an exported function.
 *
 * @param[in] mod_name The module where to search the function.
 * @param[in] library The library name.
 * @param nid The NID of the function.
 *
 * @return The function address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlHENFindFunction(const char *mod_name, const char *library, u32 nid);

/**
 * Finds an exported function in a `SceModule`.
 *
 * Good to optimize when you already have a `SceModule` module and wants to
 * find a function in that module.
 *
 * @param[in] mod The module where to search the function.
 * @param[in] library The library name.
 * @param nid The NID of the function.
 *
 * @return The function address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlHENFindFunctionInMod(SceModule *mod, const char *library, u32 nid);

/**
 * Finds an exported function in the system that matches the specified library
 * name and NID.
 *
 * @param[in] library The library name.
 * @param nid The NID of the function.
 * @param user_mods_only Set to `1` to only receive UIDs from user mode modules.
 * Set to `0` to receive UIDs from kernel and user loaded modules.
 *
 * @return The function address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlHENFindFunctionOnSystem(const char *libname, u32 nid,
                                 int user_mods_only);

/**
 * Finds an import function stub.
 *
 * @param[in] mod_name The module where to search the function.
 * @param[in] library The library name.
 * @param nid The NID of the function.
 *
 * @returns The function address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlHENFindImport(const char *mod_name, const char *library, u32 nid);

/**
 * Finds a import function stub in a `SceModule`.
 *
 * Good to optimize when you already have a `SceModule` module and wants to
 * find a stub in that module.
 *
 * @param[in] mod The module where to search the import.
 * @param[in] library The library name.
 * @param nid The nid of the function.
 *
 * @return The function address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlHENFindImportInMod(SceModule *mod, const char *libname, u32 nid);

/**
 * Hijack a function.
 *
 * Stackble function hijacking.
 *
 * @param[inout] patch_data A buffer to store data for the patch.
 * @param[in] func_addr A function address to be hijacked.
 * @param[in] patch_func A function that will take over.
 * @param[out] orig_func A pointer to a pointer, to store pointer to call
 * original (or next) function.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
void sctrlHENHijackFunction(FunctionPatchData *patch_data, void *func_addr,
                            void *patch_func, void **orig_func);

/**
 * Sets a function to be called just before module_start of a module is gonna be
 * called (useful for patching purposes)
 *
 * @param handler The function, that will receive the module structure before
 * the module is started.
 *
 * @return The previous set function (`NULL` if none);
 *
 * @note Because only one handler function is handled by HEN, you should
 * call the previous function in your code.
 *
 * @example
 * STMOD_HANDLER previous = NULL;
 *
 * int OnModuleStart(SceModule *mod);
 *
 * void somepointofmycode()
 * {
 *		previous = sctrlHENSetStartModuleHandler(OnModuleStart);
 * }
 *
 * int OnModuleStart(SceModule *mod)
 * {
 *		if (strcmp(mod->modname, "vsh_module") == 0)
 *		{
 *			// Do something with vsh module here
 *		}
 *
 *		if (!previous)
 *			return 0;
 *
 *		// Call previous handler
 *
 *		return previous(mod);
 * }
 *
 * @note The above example should be compiled with the flag -fno-pic
 *			in order to avoid problems with gp register that may
 * lead to a crash.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER new_handler);

/**
 * Creates a syscall stub for the given `function` in user-memory.
 *
 * Creating a syscall stub this way allows a kernel program to inject syscall
 * calls in a user module without overwriting a existing syscall stub in the
 * module being modified.
 *
 * @note The given `function` must be exported as syscall for it to work.
 *
 * @param function The function pointer to the function to create the syscall
 * stub
 *
 * @return The syscall stub of the given function (`> 0`). Or zero if the
 * function fails to create the stub.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
 *
 * @attention Every call to this function allocates 8 bytes of memory in the
 * user RAM, which is also the available memory for the running application. So,
 * avoid excessive use of this function.
 *
 * @note This function should be avoided at all costs, it is here for backwards
 * compatibility only.
 */
u32 sctrlHENMakeSyscallStub(void *function);

/**
 * Sets the speed for the cpu and bus.
 *
 * @param cpu The cpu speed.
 * @param bus The bus speed.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
void sctrlHENSetSpeed(int cpu, int bus);

/**
 * Sets the partition 2 and 11 memory (to unlock extra memory on partition 2) on
 * next reboot/loadexec.
 *
 * This feature is only available in PSP 2g+ and PS Vita.
 *
 * @param p2 The size in MB for the user partition. Must be `> 0`.
 * @param p8_or_p11 The size in MB for partition 8 (on PSP) or 11 (on PS VITA).
 * It can be `0`.
 *
 * @returns `0` on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 *
 * @attention [ARK] This function has been deprecated in ARK and has no effect.
 * Use sctrlHENApplyMemory instead.
 * @attention [EPI] This function will fail if `p2+p8 > 52` or `p2 == 0`.
 * @attention [EPI] This function will fail with `-1` if can't unlock (i.e.
 * pops, vsh), `-2` if already unlocked, `-3` if too late to unlock.
 */
int sctrlHENSetMemory(u32 p2, u32 p8_or_p11);

/**
 * Unlocks extra memory on partition 2 (user RAM).
 *
 * This feature is only available in PSP 2g+ and PS Vita.
 *
 * @param p2 The size in MB for the user partition. The actual value is
 * ignored but must be `> 24`.
 *
 * @return `0` on success, `-1` if can't unlock (i.e. pops, vsh, 1k, function to
 * unlock not found), `-2` if already unlocked, `-3` if too late to unlock.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlHENApplyMemory(u32 p2);

/**
 * Query system call number of `function`.
 *
 * @param[in] function A function pointer of the function to get the syscall
 * number.
 *
 * @return System call number if `>= 0`, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelQuerySystemCall(void *func_addr);

/**
 * Obtain the boot device.
 *
 * @return The boot device.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlKernelBootFrom(void);

/**
 * Changes a syscall to another function.
 *
 * This is achieved by modifying the syscall table to point to `newaddr` when
 * calling the original syscall. This means that every system call to the
 * original function (in any module) will execute the new function instead.
 *
 * @param[in] addr The address of the original function.
 * @param[in] newaddr The address of the new function.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
void sctrlHENPatchSyscall(void *addr, void *newaddr);

/**
 * Patch module by offset.
 *
 * @param[in] mod_name The module name.
 * @param inst The instruction to overwrite.
 * @param offset The module patch offset.
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
int sctrlPatchModule(char *mod_name, u32 inst, u32 offset);

/**
 * Get module text address.
 *
 * @param[in] mod_name The module name.
 *
 * @return The text address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlModuleTextAddr(char *mod_name);

/**
 * Get `sceInit` module text address.
 *
 * @note Only useful before `sceInit` exits.
 *
 * @return The `sceInit` text address, or `0` if not found.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
 */
u32 sctrlGetInitTextAddr(void);

/**
 * Set custom start module handler.
 *
 * It can be used to replace a system module.
 *
 * @param[in] func function pointer to register as custom start module handler
 *
 * @note The `func` returns `-1` to ignore the module and load the original
 * module, or new `modid` if a replace is done.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
 */
void sctrlSetCustomStartModule(int (*func)(int modid, SceSize argsize,
                                           void *argp, int *modstatus,
                                           SceKernelSMOption *opt));

/**
 * Loads a module on next reboot.
 *
 * @param module_after The path of the module which is loaded after the module
 to be loaded. The module passed to this function will be loaded just before
 that module.
 * @param buf The buffer containing the module - Don't deallocate this one. It
 has to reside in kernel memory.
 * @param size The size of the module
 * @param flags The modes in which the module should be loaded, one of
 BootLoadFlags
 *
 * @example:
 * sctrlHENLoadModuleOnReboot("/kd/usersystemlib.prx", module_buffer,
 module_size, BOOTLOAD_GAME | BOOTLOAD_POPS | BOOTLOAD_UMDEMU);
 *
 * This will load the module contained in module_buffer just before
 /kd/usersystemlib.prx in the next reboot, if the mode of next reboot is game,
 pops or umdemu
 *
 * @note Don't use too early modules in first param like "/kd/init.prx" or
 "/kd/systemctrl.prx", or your module may not load properly
 * Only one module will be loaded on reboot with this function.
 * If this function is called many times, only the last one will be considered.
 * By making a module to load itself using this function, and calling
 * `sctrlHENLoadModuleOnReboot` on module_start, a prx can cause itself to be
 resident in the modes choosen by flags.
 * If all flags are selected, the module will stay resident until a psp
 shutdown, or until `sctrlHENLoadModuleOnReboot` is not called.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
*/
void sctrlHENLoadModuleOnReboot(char *module_after, void *buf, int size,
                                int flags);

/**
 * Override rebootex.
 */
void sctrlHENSetRebootexOverride(const u8 *rebootex);

/**
 * Enable/disable NID Resolver on particular library
 *
 * @param libname The name of the library to be enabled/disabled.
 * @param enabled `0` for disabled, `!= 0` for enabled.
 *
 * @Example:
 * sctrlKernelSetNidResolver("sceImpose_driver", 0); // disable sceImpose_driver
 * resolving
 *
 * @return previous value if set, `< 0` on error
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 *
 * @attention [EPI] On Adrenaline this function always returns a error.
 */
int sctrlKernelSetNidResolver(char *libname, u32 enabled);

/**
 * Get the real unspoofed Ethernet (MAC) Address of the systems WLAN chip.
 *
 * @param[out] mac The buffer (6B) for real MAC Address
 *
 * @return `0` on success, `< 0` on error.
 */
int sctrlGetRealEthernetAddress(uint8_t *mac);

/**
 * Get the rebootex configuration.
 *
 * Needs to be casted correctly based on running CFW.
 *
 * @param config A pointer to where the config will be copied, or `NULL`.
 *
 * @return A pointer to the global rebootex configuration.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
 */
RebootexConfig *sctrlHENGetRebootexConfig(RebootexConfig *);

/**
 * Resolves the NID of a library if missing.
 *
 * @param libname The name of the library.
 * @param nid The NID to resolve.
 *
 * @return The function pointer to the resolved function, `0` otherwise.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel`
 * stubs to be available.
 */
u32 sctrlKernelResolveNid(const char *libName, u32 nid);

/**
 *  Find a import library stub table.
 *
 * @param mod The module where to search the function.
 * @param library The library name.
 *
 * @return The reference to the stub table ot NULL if not found.
 */
SceLibraryStubTable *sctrlHENFindImportLib(SceModule *mod, const char *library);

/**
 * Initialize or stop Memory Stick cache.
 *
 * @param[in] driver A driver name (e.g. `"ms"`) to initialize the cache, or
 * `NULL` to deactivate it.
 * @param cache_size The cache size to use. The caller can use one of
 * `MSCACHE_BUFSIZE_*` as suggested values.
 *
 * @return `0` on success, `< 0` on error.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stubs to be available.
 */
int sctrlMsCacheInit(const char *driver, int cache_size);

/**
 * Checks if CFW is currently loading plugin modules.
 *
 * @return `1` if the CFW is currently loading plugin modules, `0` otherwise.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stubs to be available.
 */
int sctrlIsLoadingPlugins();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSTEMCTRL_H_ */
