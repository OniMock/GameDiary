#ifndef INFERNO_CTRL_H
#define INFERNO_CTRL_H

#include <psptypes.h>

enum InfernoCachePolicy {
    INFERNO_CACHE_DISABLED = 0,
    INFERNO_CACHE_LRU = 1, // Least Recently Used
    INFERNO_CACHE_RR = 2, // Random Replacement
};

#ifdef __cplusplus
extern "C"{
#endif

/**
 * Issue a direct ISO/CSO read request to Inferno.
 *
 * @param offset within the Virtual UMD to start reading from. Absolute Offset.
 * @param ptr buffer to write data to.
 * @param data_len amount of bytes to read.
 *
 * @return amount of bytes read, <0 on error.
 */
extern int infernoIsoRead(u32 offset, void *ptr, u32 data_len);

/**
 * Set UMD disc type (enum pspUmdTypes).
 *
 * @param type of the Virtual UMD.
 * @return 0 on success, <0 on error.
 */
extern int infernoSetDiscType(int type);

/**
 * Sets Cache Policy, necessary before initializing cache.
 *
 * @param policy one of Inferno Cache Policies.
 */
extern void infernoCacheSetPolicy(int policy);

/**
 * Initialize Inferno Cache. Requires Cache Policy to be set beforehand.
 *
 * If cache size is set to 0, then cache is turned off.
 * Cache is not allowed on homebrew, it will return 0 but will not enable.
 *
 * @param cache_size size of each cache block, in bytes.
 * @param cache_num amount of cache blocks.
 * @param partition ram partition where to allocate cache.
 *
 * @return 0 on success, <0 on error.
 * @return -1 if total cache size (cache_size*cache_num) is not a multiple of 512.
 * @return -2/-3/-4 if failed to allocate cache buffers.
 * @return -10 if cache policy not set.
 *
 */
extern int infernoCacheInit(int cache_size, int cache_num, int partition);

/**
 * Sets UMD speed simulation.
 *
 * @param seek 0 to disable, >0 to enable, the higher the value the slower the seek time.
 * @param speed 0 to disable, >0 to enable, the higher the value the slower the read time.
 *
 */
extern void infernoSetUmdDelay(int seek, int speed);


#ifdef __cplusplus
}
#endif

#endif /* INFERNO_CTRL_H */