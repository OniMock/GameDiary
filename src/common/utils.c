#include "common/utils.h"
#include <pspkernel.h>
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

// Helper to copy a file
int utils_copy_file(const char *src, const char *dst) {
  SceUID f_in = sceIoOpen(src, PSP_O_RDONLY, 0777);
  if (f_in < 0)
    return -1;
  SceUID f_out = sceIoOpen(dst, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (f_out < 0) {
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
  if (f_out < 0) {
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
