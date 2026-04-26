/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

/* pspkernel.h transitively provides u8, u32, and other scalar types. */
#include <pspkernel.h>
#include <time.h>

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
 * @brief Returns the overlap in seconds between two half-open time intervals.
 *
 * Computes the intersection of [a_start, a_end) and [b_start, b_end), expressed
 * in seconds. Returns 0 if the intervals do not overlap.
 *
 * Used by the stats calculators to split a session's duration across calendar
 * boundaries (day, month, year) so each period only counts the time that
 * genuinely fell within it.
 *
 * @param a_start  Start of interval A (inclusive, UNIX seconds).
 * @param a_end    End   of interval A (exclusive, UNIX seconds).
 * @param b_start  Start of interval B (inclusive, UNIX seconds).
 * @param b_end    End   of interval B (exclusive, UNIX seconds).
 * @return Overlap duration in seconds, or 0 if no overlap.
 */
u32 utils_time_overlap_secs(time_t a_start, time_t a_end, time_t b_start, time_t b_end);

void utils_format_duration_compact(u32 seconds, char *out, size_t size);
const char* utils_get_device_prefix(void);
void utils_ensure_storage_dirs(const char* prefix);

/**
 * @brief Logs an error to error.txt with timestamp.
 */
void utils_log_error(const char *module, const char *msg, int code);

/**
 * @brief Logs a trace message to error.txt
 */
void utils_log_trace(const char *module, const char *msg);

#ifdef GDIARY_PLUGIN
/**
 * @brief Captures/extracts a game icon to the destination directory.
 * Only available in Plugin context.
 */
void utils_capture_icon(const char *game_id, u8 category, const char *dest_dir, const char *executable_path);
#endif

/**
 * @brief Sets global context information for the error logs.
 */
void utils_set_log_context(const char *game_id);

#endif // _COMMON_UTILS_H_
