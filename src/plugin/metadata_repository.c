#include "plugin/metadata_repository.h"
#include "plugin/apitype.h"
#include "common/common.h"
#include "common/sfo_parser.h"
#include "common/utils.h"
#include <pspkernel.h>
#include <pspsdk/kubridge.h>
#include <pspsdk/systemctrl.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Resets metadata to fallback values.
 */
static void set_default_metadata(GameMetadata *metadata) {
    strncpy(metadata->game_id, "UNKNOWN-00000", sizeof(metadata->game_id) - 1);
    metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
    
    strncpy(metadata->game_name, "Unknown Game", sizeof(metadata->game_name) - 1);
    metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
}

/**
 * @brief Attempts to fetch ID and Title from the System Control (PSP/ISO/HB).
 *        This uses the kernel-mode sctrlGetInitPARAM API.
 */
static void fetch_system_metadata(GameMetadata *metadata) {
    u16 param_type = 0;
    u32 param_len = 0;
    char param_buf[256];

    // Try to get DISC_ID
    if (sctrlGetInitPARAM("DISC_ID", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < sizeof(metadata->game_id)) {
            memcpy(metadata->game_id, param_buf, param_len);
            metadata->game_id[param_len] = '\0';
        }
    }

    // Try to get TITLE
    if (sctrlGetInitPARAM("TITLE", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < sizeof(metadata->game_name)) {
            memcpy(metadata->game_name, param_buf, param_len);
            metadata->game_name[param_len] = '\0';
        }
    }
}

/**
 * @brief Extracts metadata specifically from PS1 EBOOT.PBP files.
 */
static void fetch_ps1_metadata(GameMetadata *metadata, const char *path) {
    if (!path || path[0] == '\0') return;

    pbp_read_sfo_string(path, "DISC_ID", metadata->game_id, sizeof(metadata->game_id));
    pbp_read_sfo_string(path, "TITLE", metadata->game_name, sizeof(metadata->game_name));

    // Fallback if TITLE is missing (common in some converted PS1 games)
    if (metadata->game_name[0] == '\0') {
        if (metadata->game_id[0] != '\0') {
            snprintf(metadata->game_name, sizeof(metadata->game_name), "PS1: %s", metadata->game_id);
        } else {
            // Last resort: use numeric part of path or filename
            const char *filename = strrchr(path, '/');
            filename = filename ? filename + 1 : path;
            snprintf(metadata->game_name, sizeof(metadata->game_name), "PT: %.50s", filename);
        }
    }
}

/**
 * @brief Generates a unique ID for Homebrews using a hash of their title.
 */
static void fetch_homebrew_fallback_id(GameMetadata *metadata) {
    if (strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
        snprintf(metadata->game_id, sizeof(metadata->game_id), "HBX-%08X",
                 (unsigned int)hash_string(metadata->game_name));
    }
}

int metadata_fetch(GameMetadata *metadata) {
    int apitype = sceKernelInitApitype();
    metadata->category = apitype_detect_category(apitype);
    
    snprintf(metadata->apitype_str, sizeof(metadata->apitype_str), "0x%03X", (unsigned int)apitype);
    set_default_metadata(metadata);
    
    // Resolve executable path
    memset(metadata->file_path, 0, sizeof(metadata->file_path));
    if (kuKernelInitFileName(metadata->file_path) < 0) {
        metadata->file_path[0] = '\0';
    }

    // Branching based on category
    switch (metadata->category) {
        case CAT_PS1:
            fetch_ps1_metadata(metadata, metadata->file_path);
            break;

        case CAT_PSP:
        case CAT_HOMEBREW:
            fetch_system_metadata(metadata);
            if (metadata->category == CAT_HOMEBREW) {
                fetch_homebrew_fallback_id(metadata);
            }
            break;

        default:
            // VSH or UNKNOWN
            break;
    }

    return 1;
}

int metadata_fetch_from_umd(GameMetadata *metadata) {
    SceUID fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0);
    if (fd < 0) return 0;

    char buf[64];
    memset(buf, 0, sizeof(buf));
    int bytes = sceIoRead(fd, buf, sizeof(buf) - 1);
    sceIoClose(fd);

    if (bytes <= 0) return 0;

    // Parse DISC_ID from "DISC_ID|..." format
    int j = 0;
    for (int i = 0; i < bytes && j < (int)sizeof(metadata->game_id) - 1; i++) {
        if (buf[i] == '|' || buf[i] == '\r' || buf[i] == '\n') break;
        if (buf[i] != '-') {
            metadata->game_id[j++] = buf[i];
        }
    }
    metadata->game_id[j] = '\0';
    metadata->category = CAT_PSP;

    // Try to get Title from UMD SFO
    sfo_read_string("disc0:/PSP_GAME/PARAM.SFO", "TITLE", metadata->game_name, sizeof(metadata->game_name));
    
    return 1;
}

