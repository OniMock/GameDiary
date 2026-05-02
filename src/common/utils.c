/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file utils.c
 * @brief Utility functions implementation.
 */

#include "common/utils.h"
#include "common/db_schema.h"
#include <pspkernel.h>
#include <pspsdk/systemctrl.h>
#include <psprtc.h>
#include <stdio.h>
#include <string.h>



u32 hash_string(const char *str) {
  u32 hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

// Safely read a 32-bit little-endian value from a potentially unaligned buffer
u32 utils_get_u32_le(const u8 *p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

u32 utils_get_timestamp(void) {
  u64 tick;
  sceRtcGetCurrentTick(&tick);
  // tick is microseconds since 0001-01-01.
  // UNIX epoch is 1970-01-01. Difference is roughly 62135596800 seconds.
  return (u32)((tick / 1000000ULL) - 62135596800ULL);
}

u32 utils_get_time_ms(void) {
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return (u32)(tick / (sceRtcGetTickResolution() / 1000));
}

u32 utils_time_overlap_secs(time_t a_start, time_t a_end, time_t b_start, time_t b_end) {
  /* Returns the length of the intersection of [a_start, a_end) and [b_start, b_end).
   * Both intervals are half-open (start inclusive, end exclusive).
   * Used by all period-based stats calculators to split session duration
   * across calendar day/month/year boundaries. */
  time_t s = (a_start > b_start) ? a_start : b_start;
  time_t e = (a_end   < b_end  ) ? a_end   : b_end;
  return (e > s) ? (u32)(e - s) : 0u;
}

void utils_format_duration_compact(u32 seconds, char *out, size_t size) {
  (void)seconds;
  (void)out;
  (void)size;
}

const char *utils_get_device_prefix(void) {
  static const char *cached = NULL;
  if (cached)
    return cached;

  SceUID fd;

  // 1. Check for existing database on Memory Stick (ms0) - Standard Priority
  fd = sceIoOpen("ms0:" GDIARY_BASE_DIR "/" GDIARY_DB_DIR "/" GAMES_DAT, PSP_O_RDONLY, 0);
  if (fd >= 0) {
    sceIoClose(fd);
    cached = "ms0:";
    return cached;
  }

  // 2. Check for existing database on Internal Storage (ef0)
  fd = sceIoOpen("ef0:" GDIARY_BASE_DIR "/" GDIARY_DB_DIR "/" GAMES_DAT, PSP_O_RDONLY, 0);
  if (fd >= 0) {
    sceIoClose(fd);
    cached = "ef0:";
    return cached;
  }

  // 3. Detect if ef0 exists (PSP Go internal memory)
  SceUID dir = sceIoDopen("ef0:/");
  if (dir >= 0) {
    sceIoDclose(dir);
    cached = "ef0:";
    return cached;
  }

  // 4. Default Fallback
  cached = "ms0:";
  return cached;
}

void utils_ensure_storage_dirs(const char *prefix) {
  char path[128];

  // 1. Create root GameDiary folder
  snprintf(path, sizeof(path), "%s" GDIARY_BASE_DIR, prefix);
  sceIoMkdir(path, 0777);

  // 2. Create db subfolder
  snprintf(path, sizeof(path), "%s" GDIARY_BASE_DIR "/db", prefix);
  sceIoMkdir(path, 0777);
}

// Helper to copy a file
int utils_copy_file(const char *src, const char *dst) {
  SceUID f_in = sceIoOpen(src, PSP_O_RDONLY, 0777);
  if (f_in < 0)
    return -1;
  SceUID f_out = sceIoOpen(dst, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (f_out < 0) f_out = sceIoOpen(dst, PSP_O_RDWR | PSP_O_CREAT, 0777);

  if (f_out < 0) {
    utils_log_error("utils", "Failed to create/open icon dest file in copy", f_out);
    sceIoClose(f_in);
    return -2;
  }
  char buf[2048];
  int bytes;
  while ((bytes = sceIoRead(f_in, buf, sizeof(buf))) > 0) {
    sceIoWrite(f_out, buf, bytes);
  }
  sceIoClose(f_in);
  sceIoClose(f_out);
  return 0;
}

// Helper to extract ICON0.PNG from PBP
int utils_extract_pbp_icon(const char *pbp_path, const char *dst) {
  SceUID f_in = sceIoOpen(pbp_path, PSP_O_RDONLY, 0777);
  if (f_in < 0) {
    return -1;
  }

  u8 header[40];
  if (sceIoRead(f_in, header, 40) != 40) {
    sceIoClose(f_in);
    return -2;
  }

  if (header[0] != 0x00 || header[1] != 'P' || header[2] != 'B' || header[3] != 'P') {
    sceIoClose(f_in);
    return -3;
  }

  u32 icon0_ofs = utils_get_u32_le(&header[12]);
  if (icon0_ofs == 0) {
    sceIoClose(f_in);
    return -4;
  }

  u32 next_ofs = 0;
  for (int i = 16; i <= 36; i += 4) {
    u32 off = utils_get_u32_le(&header[i]);
    if (off > icon0_ofs) {
      next_ofs = off;
      break;
    }
  }

  if (next_ofs == 0) {
    sceIoClose(f_in);
    return -5;
  }

  u32 size = next_ofs - icon0_ofs;
  if (size == 0 || size > 1024 * 1024) {
    sceIoClose(f_in);
    return -6;
  }

  SceUID f_out = sceIoOpen(dst, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (f_out < 0) f_out = sceIoOpen(dst, PSP_O_RDWR | PSP_O_CREAT, 0777);

  if (f_out < 0) {
    utils_log_error("utils", "Failed to create/open icon dest file in extract", f_out);
    sceIoClose(f_in);
    return -7;
  }

  sceIoLseek(f_in, icon0_ofs, PSP_SEEK_SET);
  char buf[2048];
  u32 total_read = 0;
  while (total_read < size) {
    u32 to_read = (size - total_read > sizeof(buf)) ? sizeof(buf) : (size - total_read);
    int bytes = sceIoRead(f_in, buf, to_read);
    if (bytes <= 0)
      break;
    sceIoWrite(f_out, buf, bytes);
    total_read += bytes;
  }

  sceIoClose(f_in);
  sceIoClose(f_out);
  return 0;
}

static char g_log_game_id[32] = "N/A";
static char g_log_cfw_info[64] = "FW: Unknown | HEN: Unknown";
static int g_log_context_set = 0;

void utils_set_log_context(const char *game_id) {
    if (game_id && game_id[0] != '\0') {
        snprintf(g_log_game_id, sizeof(g_log_game_id), "%s", game_id);
    }

    if (!g_log_context_set) {
        int fw = sceKernelDevkitVersion();
#ifdef GDIARY_PLUGIN
        u32 hen = sctrlHENGetVersion();
        snprintf(g_log_cfw_info, sizeof(g_log_cfw_info), "FW: %08X | HEN: %08X", fw, (unsigned int)hen);
#else
        snprintf(g_log_cfw_info, sizeof(g_log_cfw_info), "FW: %08X | AppMode", fw);
#endif
        g_log_context_set = 1;
    }
}

void utils_log_error(const char *module, const char *msg, int code) {
  char log_path[128];
  snprintf(log_path, sizeof(log_path), "%s" GDIARY_BASE_DIR "/error.txt", utils_get_device_prefix());

  SceUID fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd < 0) fd = sceIoOpen(log_path, PSP_O_RDWR | PSP_O_CREAT, 0777);

  if (fd >= 0) {
    char buf[256];
    u32 ts = utils_get_timestamp();
    if (!g_log_context_set) utils_set_log_context(NULL);
    int len = snprintf(buf, sizeof(buf), "[%u] [ERR] [%s] %s (Code: %d) | Game: %s | Sys: %s\r\n",
                       (unsigned int)ts, module, msg, code, g_log_game_id, g_log_cfw_info);
    sceIoWrite(fd, buf, len);
    sceIoClose(fd);
  }
}

void utils_log_trace(const char *module, const char *msg) {
  char log_path[128];
  snprintf(log_path, sizeof(log_path), "%s" GDIARY_BASE_DIR "/error.txt", utils_get_device_prefix());

  SceUID fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
  if (fd < 0) fd = sceIoOpen(log_path, PSP_O_RDWR | PSP_O_CREAT, 0777);

  if (fd >= 0) {
    char buf[256];
    u32 ts = utils_get_timestamp();
    if (!g_log_context_set) utils_set_log_context(NULL);
    int len = snprintf(buf, sizeof(buf), "[%u] [TRC] [%s] %s | Game: %s | Sys: %s\r\n",
                       (unsigned int)ts, module, msg, g_log_game_id, g_log_cfw_info);
    sceIoWrite(fd, buf, len);
    sceIoClose(fd);
  }
}

