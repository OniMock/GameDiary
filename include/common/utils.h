#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

/* pspkernel.h transitively provides u8, u32, and other scalar types. */
#include <pspkernel.h>

/**
 * @brief Simple djb2 hash function for strings.
 * @param str Null-terminated input string.
 * @return 32-bit hash value.
 */
u32 hash_string(const char *str);

/**
 * @brief Copies a file from src to dst using sceIo.
 * @return 0 on success, negative on error.
 */
int utils_copy_file(const char *src, const char *dst);

/**
 * @brief Extracts ICON0.PNG embedded in a PBP archive to dst.
 * @return 0 on success, negative on error.
 */
int utils_extract_pbp_icon(const char *pbp_path, const char *dst);

/**
 * @brief Safely reads a 32-bit little-endian value from an unaligned buffer.
 */
u32 utils_get_u32_le(const u8 *p);

/**
 * @brief Gets the current UNIX timestamp using the PSP RTC.
 * Syncs app and plugin time logic.
 */
u32 utils_get_timestamp(void);

/**
 * @brief Formats seconds into a human-readable "h m" string.
 */
void utils_format_time(u32 seconds, char *out, size_t size);

#ifdef GDIARY_PLUGIN
/**
 * @brief Captures/extracts a game icon to the destination directory.
 * Only available in Plugin context.
 */
void utils_capture_icon(const char *game_id, u8 category, const char *dest_dir, const char *executable_path);
#endif

#endif // _COMMON_UTILS_H_
