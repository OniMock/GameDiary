#include "common/sfo_parser.h"
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

// Shared internal function to parse SFO from an open file at a specific offset
static int parse_sfo_from_fd(SceUID fd, u32 sfo_offset, const char *target_key,
                             char *out, int out_max) {
  if (sceIoLseek(fd, sfo_offset, PSP_SEEK_SET) < 0)
    return 0;

  u8 header[20];
  if (sceIoRead(fd, header, 20) != 20)
    return 0;

  // Check magic '\0PSF'
  if (header[0] != 0x00 || header[1] != 'P' || header[2] != 'S' ||
      header[3] != 'F') {
    return 0;
  }

  u32 key_ofs = *(u32 *)&header[8];
  u32 data_ofs = *(u32 *)&header[12];
  u32 count = *(u32 *)&header[16];

  int found = 0;
  for (u32 i = 0; i < count && i < 256; i++) {
    sceIoLseek(fd, sfo_offset + 20 + i * 16, PSP_SEEK_SET);
    u8 entry[16];
    if (sceIoRead(fd, entry, 16) != 16)
      break;

    u16 k_ofs = *(u16 *)&entry[0];
    u32 d_ofs = *(u32 *)&entry[12];
    u32 d_max = *(u32 *)&entry[8];

    char key_str[32];
    sceIoLseek(fd, sfo_offset + key_ofs + k_ofs, PSP_SEEK_SET);
    if (sceIoRead(fd, key_str, sizeof(key_str) - 1) <= 0)
      continue;
    key_str[sizeof(key_str) - 1] = '\0';

    if (strcmp(key_str, target_key) == 0) {
      sceIoLseek(fd, sfo_offset + data_ofs + d_ofs, PSP_SEEK_SET);
      u32 limit = (u32)(out_max - 1);
      u32 size = (d_max < limit) ? d_max : limit;
      int bytes = sceIoRead(fd, out, (int)size);
      if (bytes > 0) {
        out[bytes] = '\0';
        found = 1;
      }
      break;
    }
  }

  return found;
}

int sfo_read_string(const char *path, const char *target_key, char *out,
                    int out_max) {
  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
  if (fd < 0)
    return 0;

  int result = parse_sfo_from_fd(fd, 0, target_key, out, out_max);
  sceIoClose(fd);
  return result;
}

int pbp_read_sfo_string(const char *path, const char *target_key, char *out,
                        int out_max) {
  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
  if (fd < 0)
    return 0;

  u8 pbp_head[12];
  if (sceIoRead(fd, pbp_head, 12) != 12) {
    sceIoClose(fd);
    return 0;
  }

  // Check '\0PBP'
  if (pbp_head[0] != 0x00 || pbp_head[1] != 'P' || pbp_head[2] != 'B' ||
      pbp_head[3] != 'P') {
    sceIoClose(fd);
    return 0;
  }

  u32 sfo_offset = *(u32 *)&pbp_head[8];
  int result = parse_sfo_from_fd(fd, sfo_offset, target_key, out, out_max);

  sceIoClose(fd);
  return result;
}
