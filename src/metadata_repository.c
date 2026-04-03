#include "metadata_repository.h"
#include "apitype.h"
#include "common.h"
#include "sfo_parser.h"
#include "utils.h"
#include <pspkernel.h>
#include <pspsdk/kubridge.h>
#include <stdio.h>
#include <string.h>

static void set_default_metadata(GameMetadata *metadata) {
  strncpy(metadata->game_id, "UNKNOWN-00000", 15);
  metadata->game_id[15] = '\0';
  strncpy(metadata->game_name, "Unknown Game", 63);
  metadata->game_name[63] = '\0';
}

int metadata_fetch(GameMetadata *metadata) {
  u16 param_type = 0;
  u32 param_len = 0;
  char param_buf[256];

  int apitype = sceKernelInitApitype();
  metadata->category = apitype_detect_category(apitype);
  snprintf(metadata->apitype_str, sizeof(metadata->apitype_str), "0x%03X",
           (unsigned int)apitype);

  set_default_metadata(metadata);

  // 1. Fetch PARAMs from System (PSP/ISO/Homebrew)
  if (metadata->category == CAT_PSP || metadata->category == CAT_HOMEBREW) {
    if (sctrlGetInitPARAM("DISC_ID", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 16) {
        strncpy(metadata->game_id, param_buf, param_len);
        metadata->game_id[param_len] = '\0';
      }
    }

    if (sctrlGetInitPARAM("TITLE", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 64) {
        strncpy(metadata->game_name, param_buf, param_len);
        metadata->game_name[param_len] = '\0';
      }
    }
  }

  // 2. PS1 Special Handling (Parsing actual EBOOT.PBP)
  if (metadata->category == CAT_PS1) {
    char eboot_path[256];
    if (kuKernelInitFileName(eboot_path) >= 0 && eboot_path[0] != '\0') {
      pbp_read_sfo_string(eboot_path, "DISC_ID", metadata->game_id, 16);
      pbp_read_sfo_string(eboot_path, "TITLE", metadata->game_name, 64);

      if (metadata->game_name[0] == '\0') {
        if (metadata->game_id[0] != '\0') {
          snprintf(metadata->game_name, 63, "PS1: %s", metadata->game_id);
        } else {
          snprintf(metadata->game_name, 63, "PT: %.58s", eboot_path);
        }
        metadata->game_name[63] = '\0';
      }
    }
  }

  // 3. Homebrew Fallback hash
  if (metadata->category == CAT_HOMEBREW &&
      strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
    snprintf(metadata->game_id, 16, "HBX-%08X",
             (unsigned int)hash_string(metadata->game_name));
  }

  return 1;
}

int metadata_fetch_from_umd(GameMetadata *metadata) {
  SceUID fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0);
  if (fd >= 0) {
    char buf[64];
    memset(buf, 0, sizeof(buf));
    int bytes = sceIoRead(fd, buf, sizeof(buf) - 1);
    sceIoClose(fd);

    if (bytes > 0) {
      int j = 0;
      for (int i = 0; i < bytes && j < 15; i++) {
        if (buf[i] == '|' || buf[i] == '\r' || buf[i] == '\n')
          break;
        if (buf[i] != '-') {
          metadata->game_id[j++] = buf[i];
        }
      }
      metadata->game_id[j] = '\0';
      metadata->category = CAT_PSP;

      // Try to get Title from UMD SFO
      sfo_read_string("disc0:/PSP_GAME/PARAM.SFO", "TITLE", metadata->game_name,
                      64);
      return 1;
    }
  }
  return 0;
}
