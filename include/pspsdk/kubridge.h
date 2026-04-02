#ifndef __KULIBRARY__
#define __KULIBRARY__

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspsysmem_kernel.h>
#include <pspctrl.h>
#include <psputilsforkernel.h>


// interface for passing arguments to kernel functions
typedef struct KernelCallArg {
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    uint32_t arg6;
    uint32_t arg7;
    uint32_t arg8;
    uint32_t arg9;
    uint32_t arg10;
    uint32_t arg11;
    uint32_t arg12;
    uint32_t ret1;
    uint32_t ret2;
} KernelCallArg;

/**
 * Functions to let user mode access certain functions only available in
 * kernel mode
*/

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * Load a module using ModuleMgrForKernel.
 *
 * @param[in] path The path to the module to load.
 * @param flags Unused. Always `0`.
 * @param[in] option A pointer to a `SceKernelLMOption` structure. It can be `NULL`.
 *
 * @return The UID of the loaded module on success, otherwise one of `PspKernelErrorCodes`.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
SceUID kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option);


/**
 * Load a module with a specific apitype
 *
 * @param apitype The apitype.
 * @param[in] path The path to the module to load.
 * @param flags Unused. Always `0`.
 * @param[in] option  Pointer to a `SceKernelLMOption` structure. It can be `NULL`.
 *
 * @return The UID of the loaded module on success, otherwise one of `PspKernelErrorCodes`.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
SceUID kuKernelLoadModuleWithApitype2(int apitype, const char *path, int flags, SceKernelLMOption *option);

/**
 * Gets the API type.
 *
 * @return The API type in which the system has booted.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
int kuKernelInitApitype();

/**
 * Gets the filename of the executable to be launched after all modules of the API.
 *
 * @param[out] initfilename The buffer to receive the initfilename.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
int kuKernelInitFileName(char *initfilename);

/**
 * Gets the device in which the application was launched.
 *
 * @return The device code. One of `SceBootMediumType` values.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
int kuKernelBootFrom();

/**
 * Get the key configuration in which the system has booted.
 *
 * @return The key configuration code, one of `SceApplicationType` values.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
int kuKernelInitKeyConfig();

/**
 * Get the user level of the current thread.
 *
 * @return The user level, `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelGetUserLevel(void);

/**
 * Set the protection of a block of ddr memory.
 *
 * @param[in] addr The address to set protection on.
 * @param size The size of the block to protect.
 * @param prot The protection bitmask.
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelSetDdrMemoryProtection(void *addr, int size, int prot);

/**
 * Gets the model of the PSP from user mode.
 *
 * This function is available since 3.60 M33.
 * In previous version, use the kernel function sceKernelGetModel
 *
 * @return One of `PspModel` values
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
int kuKernelGetModel(void);

/**
 * Find module by name.
 *
 * @param[in] modname The name of the module.
 * @param[out] mod A pointer to receive the module structure.
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelFindModuleByName(char *modname, SceModule *mod);

/**
 * Find module by arbitrary memory address.
 *
 * @param[in] addr The memory address somewhere inside the module
 * @param[out] mod A pointer to receive the module structure.
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelFindModuleByAddress(void *addr, SceModule *mod);

/**
 * Invalidate the entire instruction cache.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
void kuKernelIcacheInvalidateAll(void);

/**
 * Read 4 bytes from memory (with kernel memory access).
 *
 * @param[in] addr The address to read. It must have 4 bytes alignment.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
unsigned int kuKernelPeekw(void *addr);

/**
 * Write 4 bytes to memory (with kernel memory access).
 *
 * @param[in] addr The address to write. It must have 4 bytes alignment.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
void kuKernelPokew(void *addr, unsigned int value);

/**
 * memcpy (with kernel memory access).
 *
 * @param dest The destination address.
 * @param src The source address.
 * @param num The copy bytes count.
 *
 * @return The destination address.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
void *kuKernelMemcpy(void *dest, const void *src, size_t num);

/**
 * Call a kernel function with kernel privilege.
 *
 * @param[in] func_addr The kernel function address.
 * @param[inout] args The kernel arguments and return values
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelCall(void *func_addr, struct KernelCallArg *args);

/**
 * Call a kernel function with kernel privilege and extended stack.
 *
 * @param[in] func_addr The kernel function address.
 * @param[inout] args The kernel arguments and return values.
 * @param stack_size The stack size to use.
 *
 * @return `< 0` on error.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
 */
int kuKernelCallExtendStack(void *func_addr, struct KernelCallArg *args, int stack_size);

/**
 * Gets the current UMD file.
 *
 * @param[out] umdfile A buffer to receive the current UMD file data.
 * @param size The size of the `umdfile` buffer.
 *
 * @attention Requires linking to `pspkubridge` stub to be available.
*/
void kuKernelGetUmdFile(char *umdfile, int size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KULIBRARY__ */
