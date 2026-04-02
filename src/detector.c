#include "common.h"

char g_game_id[16];
char g_game_name[64];
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

    // Default setup
    strncpy(g_game_id, "UNKNOWN-00000", 15);
    g_game_id[15] = '\0';
    strncpy(g_game_name, "Unknown Game", 63);
    g_game_name[63] = '\0';
    g_category = CAT_UNKNOWN;

    // 1. Get DISC_ID
    if (sctrlGetInitPARAM("DISC_ID", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < 16) {
            strncpy(g_game_id, param_buf, param_len);
            g_game_id[param_len] = '\0';
        }
    }

    // 2. Get TITLE
    if (sctrlGetInitPARAM("TITLE", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < 64) {
            strncpy(g_game_name, param_buf, param_len);
            g_game_name[param_len] = '\0';
        }
    }

    // 3. Get CATEGORY to differentiate Homebrew, PSP Game, PS1
    if (sctrlGetInitPARAM("CATEGORY", &param_type, &param_len, param_buf) >= 0) {
        if (strncmp(param_buf, "ME", 2) == 0) {
            g_category = CAT_PS1;
        } else if (strncmp(param_buf, "UG", 2) == 0) {
            g_category = CAT_PSP; // Game
        } else if (strncmp(param_buf, "MG", 2) == 0) {
            g_category = CAT_HOMEBREW;
        } else {
            g_category = CAT_PSP;
        }
    }

    // Check if POPS manually (just in case SFO is weird)
    int apitype = sceKernelInitApitype();
    if (apitype == 0x143 /* POPS */) {
        g_category = CAT_PS1;
    }

    // Fallback ID/Name for Homebrew without SFO
    if (strcmp(g_game_id, "UNKNOWN-00000") == 0) {
        g_category = CAT_HOMEBREW;
        // Generate a pseudo-ID based on TITLE
        snprintf(g_game_id, 16, "HBX-%08X", (unsigned int)hash_string(g_game_name));
    }
}

void detector_init_late(void) {
    if (strcmp(g_game_id, "UNKNOWN-00000") == 0 || strncmp(g_game_id, "HBX-", 4) == 0) {
        SceUID fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0);
        if (fd >= 0) {
            char buf[64];
            memset(buf, 0, sizeof(buf));
            int bytes = sceIoRead(fd, buf, sizeof(buf) - 1);
            sceIoClose(fd);

            if (bytes > 0) {
                int j = 0;
                for (int i = 0; i < bytes && j < 15; i++) {
                    if (buf[i] == '|' || buf[i] == '\r' || buf[i] == '\n') break;
                    if (buf[i] != '-') {
                        g_game_id[j++] = buf[i];
                    }
                }
                g_game_id[j] = '\0';
                g_category = CAT_PSP;
            }
        }
    }
}
