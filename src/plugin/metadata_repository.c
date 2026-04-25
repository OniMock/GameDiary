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
 * @file metadata_repository.c
 * @brief Metadata repository implementation.
 */

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
 * @brief Reads TITLE and DISC_ID from the SFO embedded in a homebrew EBOOT.PBP.
 *
 * sctrlGetInitPARAM often returns empty strings for homebrews because the kernel
 * does not populate the PARAM cache the same way it does for UMD/ISO titles.
 * This function directly parses the PBP header and embedded SFO, which is the
 * same approach already used for PS1 EBOOTs.
 *
 * If the embedded SFO also has no usable TITLE (e.g. a minimal homebrew),
 * the game folder name is extracted from the executable path as a last resort.
 */
static void fetch_homebrew_sfo_metadata(GameMetadata *metadata) {
    if (metadata->file_path[0] == '\0') return;

    /* --- 1. First priority: Use the game folder name from the path ---
     * Users often name their folders cleanly (e.g. "Super Mario 64").
     * This avoids dirty embedded SFO titles like "sm64-port d98e233-dirty". */
    if (metadata->game_name[0] == '\0' || strcmp(metadata->game_name, "Unknown Game") == 0) {
        char path_copy[256];
        strncpy(path_copy, metadata->file_path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        char *last_slash = strrchr(path_copy, '/');
        if (last_slash) {
            *last_slash = '\0';
            char *prev_slash = strrchr(path_copy, '/');
            const char *dir_name = prev_slash ? prev_slash + 1 : path_copy;
            if (dir_name[0] != '\0') {
                strncpy(metadata->game_name, dir_name, sizeof(metadata->game_name) - 1);
                metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
            }
        }
    }

    /* --- 2. Fallback: Try TITLE from the SFO embedded inside EBOOT.PBP ---
     * Only if folder name extraction failed or resulted in something generic like "GAME" */
    if (metadata->game_name[0] == '\0' || strcmp(metadata->game_name, "Unknown Game") == 0 || strcmp(metadata->game_name, "GAME") == 0) {
        char title_buf[128];
        title_buf[0] = '\0';
        if (pbp_read_sfo_string(metadata->file_path, "TITLE", title_buf, sizeof(title_buf)) && title_buf[0] != '\0') {
            strncpy(metadata->game_name, title_buf, sizeof(metadata->game_name) - 1);
            metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
        }
    }

    /* --- 3. Try DISC_ID from the same embedded SFO --- */
    if (strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
        char id_buf[16];
        id_buf[0] = '\0';
        if (pbp_read_sfo_string(metadata->file_path, "DISC_ID", id_buf, sizeof(id_buf)) && id_buf[0] != '\0') {
            // Reject lazy SFO copies! Many homebrew ports (like Mario64) use copied UMD PARAM.SFO files
            // (e.g. LocoRoco UCJS10041). If it starts with standard Sony regions (UC, UL, NP), 
            // reject it so GameDiary forces a unique HBX- hash based on the resolved name.
            if (strncmp(id_buf, "UC", 2) != 0 && strncmp(id_buf, "UL", 2) != 0 && strncmp(id_buf, "NP", 2) != 0) {
                strncpy(metadata->game_id, id_buf, sizeof(metadata->game_id) - 1);
                metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
            }
        }
    }
}

/**
 * @brief Generates a unique ID for Homebrews using a hash of their title.
 */
static void fetch_homebrew_fallback_id(GameMetadata *metadata) {
    if (strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
        snprintf(metadata->game_id, sizeof(metadata->game_id), "HBX%08X",
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
            // For official PSP games, we trust the system metadata (DISC_ID/TITLE)
            fetch_system_metadata(metadata);
            break;

        case CAT_HOMEBREW:
            // For Homebrews, we first try system metadata
            fetch_system_metadata(metadata);
            
            // Critical CFW spoofing fix:
            // ISO drivers spoof DISC_ID to LocoRoco (UCJS10041). Reject it for HBs.
            // Check if the ID looks like a standard Sony ID (UC/UL/NP)
            if (strncmp(metadata->game_id, "UC", 2) == 0 || 
                strncmp(metadata->game_id, "UL", 2) == 0 || 
                strncmp(metadata->game_id, "NP", 2) == 0) {
                strncpy(metadata->game_id, "UNKNOWN-00000", sizeof(metadata->game_id) - 1);
            }

            /* Resolve name from folder and PBP SFO, then hash if needed */
            fetch_homebrew_sfo_metadata(metadata);
            fetch_homebrew_fallback_id(metadata);
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

