#ifndef _UTILS_H_
#define _UTILS_H_

#include <psptypes.h>

/**
 * @brief Simple hash function for strings (djb2).
 */
u32 hash_string(const char *str);

#endif // _UTILS_H_
