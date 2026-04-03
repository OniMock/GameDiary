#include "detector.h"
#include "common.h"
#include "apitype.h"

char g_game_id[16];
char g_game_name[64];
char g_apitype_str[8];
u8 g_category;

// Fallback hashes for apps without TITLE/DISC_ID
static u32 hash_string(const char *str) {
  u32 hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

void detector_init(void) {
  u16 param_type = 0;
  u32 param_len = 0;
  char param_buf[256];

  // 1. Detect Category via apitype
  int apitype = sceKernelInitApitype();
  g_category = apitype_detect_category(apitype);
  snprintf(g_apitype_str, sizeof(g_apitype_str), "0x%03X", (unsigned int)apitype);

  // Default setup
  strncpy(g_game_id, "UNKNOWN-00000", 15);
  g_game_id[15] = '\0';
  strncpy(g_game_name, "Unknown Game", 63);
  g_game_name[63] = '\0';

  // 2. Fetch PARAMs (works for most games: PSP/ISO and Homebrew)
  // For PS1 we don't use this as the launcher often spoofs the parameters.
  if (g_category == CAT_PSP || g_category == CAT_HOMEBREW) {
    if (sctrlGetInitPARAM("DISC_ID", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 16) {
        strncpy(g_game_id, param_buf, param_len);
        g_game_id[param_len] = '\0';
      }
    }

    if (sctrlGetInitPARAM("TITLE", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 64) {
        strncpy(g_game_name, param_buf, param_len);
        g_game_name[param_len] = '\0';
      }
    }
  }

  // 3. Category-specific fallbacks
  if (g_category == CAT_HOMEBREW) {
    if (strcmp(g_game_id, "UNKNOWN-00000") == 0) {
      // Generate a pseudo-ID based on TITLE
      snprintf(g_game_id, 16, "HBX-%08X", (unsigned int)hash_string(g_game_name));
    }
  } else if (g_category == CAT_PS1) {
    // For PS1 (POPS), sctrlGetInitPARAM often returns incorrect information
    // like generic Sony strings spoofed by the emulator.
    // So we use sctrlGetSfoPARAM with NULL to read directly from the current EBOOT.PBP
    if (sctrlGetSfoPARAM(NULL, "DISC_ID", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 16) {
        strncpy(g_game_id, param_buf, param_len);
        g_game_id[param_len] = '\0';
      }
    }
    
    // Some EBOOTs use TITLE, shouldn't hurt to try getting it too.
    if (sctrlGetSfoPARAM(NULL, "TITLE", &param_type, &param_len, param_buf) >= 0) {
      if (param_len > 0 && param_len < 64) {
        strncpy(g_game_name, param_buf, param_len);
        g_game_name[param_len] = '\0';
      }
    }
  }
}

// Helper to manually parse SFO file
static int parse_sfo_string(const char *path, const char *target_key, char *out,
                            int out_max) {
  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
  if (fd < 0)
    return 0;

  u8 header[20];
  if (sceIoRead(fd, header, 20) != 20) {
    sceIoClose(fd);
    return 0;
  }

  // Check magic '\0PSF'
  if (header[0] != 0x00 || header[1] != 'P' || header[2] != 'S' ||
      header[3] != 'F') {
    sceIoClose(fd);
    return 0;
  }

  u32 key_ofs = *(u32 *)&header[8];
  u32 data_ofs = *(u32 *)&header[12];
  u32 count = *(u32 *)&header[16];

  // Read entries (16 bytes each)
  int found = 0;
  for (u32 i = 0; i < count && i < 256; i++) {
    sceIoLseek(fd, 20 + i * 16, PSP_SEEK_SET);
    u8 entry[16];
    if (sceIoRead(fd, entry, 16) != 16)
      break;

    u16 k_ofs = *(u16 *)&entry[0];
    u32 d_ofs = *(u32 *)&entry[12];
    u32 d_max = *(u32 *)&entry[8]; // param_max or param_len

    // Read key
    char key_str[32];
    sceIoLseek(fd, key_ofs + k_ofs, PSP_SEEK_SET);
    if (sceIoRead(fd, key_str, sizeof(key_str) - 1) <= 0)
      continue;
    key_str[sizeof(key_str) - 1] = '\0';

    if (strcmp(key_str, target_key) == 0) {
      sceIoLseek(fd, data_ofs + d_ofs, PSP_SEEK_SET);
      u32 limit = (u32)(out_max - 1);
      u32 size = (d_max < limit) ? d_max : limit;
      int to_read = (int)size;
      int bytes = sceIoRead(fd, out, to_read);
      if (bytes > 0) {
        out[bytes] = '\0';
        found = 1;
      }
      break;
    }
  }

  sceIoClose(fd);
  return found;
}

void detector_init_late(void) {
  int fetched_from_umd = 0;

  if (strcmp(g_game_id, "UNKNOWN-00000") == 0 ||
      strncmp(g_game_id, "HBX-", 4) == 0) {
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
            g_game_id[j++] = buf[i];
          }
        }
        g_game_id[j] = '\0';
        g_category = CAT_PSP;
        fetched_from_umd = 1;
      }
    }
  }

  // Try to fetch Name manually if it's still unknown
  if (strcmp(g_game_name, "Unknown Game") == 0 || fetched_from_umd) {
    // Look directly in UMD
    if (parse_sfo_string("disc0:/PSP_GAME/PARAM.SFO", "TITLE", g_game_name,
                         sizeof(g_game_name))) {
      // Success
    }
  }
}
