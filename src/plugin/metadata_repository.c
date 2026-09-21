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
#include "common/debug.h"
#include "plugin/overlay_notification.h"
#include <pspkernel.h>
#include <pspsdk/kubridge.h>
#include <pspsdk/systemctrl.h>
#include <stdio.h>
#include <string.h>

#define PSP_UMD_READY_FLAG 0x20
#define SCE_UMD_GET_DRIVE_STAT_NID 0x6B4A146C
#define PHYSICAL_UMD_UNSAFE_FALLBACK_DELAY_SEC 30U
#define UMD_READINESS_UNAVAILABLE (-1)
#define UMD_READINESS_NOT_READY    0
#define UMD_READINESS_READY        1

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

    debug_log("METADATA", "fetch_system_metadata: Attempting to fetch from System Control.");

    // Try to get DISC_ID
    if (sctrlGetInitPARAM("DISC_ID", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < sizeof(metadata->game_id)) {
            memcpy(metadata->game_id, param_buf, param_len);
            metadata->game_id[param_len] = '\0';
            debug_log("METADATA", "fetch_system_metadata: DISC_ID fetched successfully: '%s'", metadata->game_id);
        } else {
            debug_log("METADATA", "fetch_system_metadata: DISC_ID found but invalid length (%d)", param_len);
        }
    } else {
        debug_log("METADATA", "fetch_system_metadata: sctrlGetInitPARAM for DISC_ID failed.");
    }

    // Try to get TITLE
    if (sctrlGetInitPARAM("TITLE", &param_type, &param_len, param_buf) >= 0) {
        if (param_len > 0 && param_len < sizeof(metadata->game_name)) {
            memcpy(metadata->game_name, param_buf, param_len);
            metadata->game_name[param_len] = '\0';
            debug_log("METADATA", "fetch_system_metadata: TITLE fetched successfully: '%s'", metadata->game_name);
        } else {
            debug_log("METADATA", "fetch_system_metadata: TITLE found but invalid length (%d)", param_len);
        }
    } else {
        debug_log("METADATA", "fetch_system_metadata: sctrlGetInitPARAM for TITLE failed.");
    }
}

/**
 * @brief Extracts metadata specifically from PS1 EBOOT.PBP files.
 */
static void fetch_ps1_metadata(GameMetadata *metadata, const char *path) {
    if (!path || path[0] == '\0') {
        debug_log("METADATA", "fetch_ps1_metadata: Path is empty, skipping PS1 fetch.");
        return;
    }

    debug_log("METADATA", "fetch_ps1_metadata: Parsing SFO from PS1 EBOOT at '%s'", path);

    pbp_read_sfo_string(path, "DISC_ID", metadata->game_id, sizeof(metadata->game_id));
    pbp_read_sfo_string(path, "TITLE", metadata->game_name, sizeof(metadata->game_name));

    debug_log("METADATA", "fetch_ps1_metadata: SFO read -> DISC_ID: '%s', TITLE: '%s'", metadata->game_id, metadata->game_name);

    // Fallback if TITLE is missing (common in some converted PS1 games)
    if (metadata->game_name[0] == '\0') {
        if (metadata->game_id[0] != '\0') {
            snprintf(metadata->game_name, sizeof(metadata->game_name), "PS1: %s", metadata->game_id);
            debug_log("METADATA", "fetch_ps1_metadata: TITLE empty. Using fallback 'PS1: %s'", metadata->game_id);
        } else {
            // Last resort: use numeric part of path or filename
            const char *filename = strrchr(path, '/');
            filename = filename ? filename + 1 : path;
            snprintf(metadata->game_name, sizeof(metadata->game_name), "PT: %.50s", filename);
            debug_log("METADATA", "fetch_ps1_metadata: DISC_ID and TITLE empty. Using filename fallback: '%s'", metadata->game_name);
        }
    }
}

static int ascii_is_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int ascii_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static char ascii_tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }
    return c;
}

/**
 * @brief Sony content IDs are 9 characters: 4 letters + 5 digits (e.g. NPIA00013).
 */
static int is_sony_title_id(const char *s) {
    int i;

    if (!s) {
        return 0;
    }

    for (i = 0; i < 9; i++) {
        if (s[i] == '\0') {
            return 0;
        }
        if (i < 4) {
            if (!ascii_is_letter(s[i])) {
                return 0;
            }
        } else if (!ascii_is_digit(s[i])) {
            return 0;
        }
    }

    return s[9] == '\0';
}

static int is_sony_region_prefix(const char *id) {
    return strncmp(id, "UC", 2) == 0 ||
           strncmp(id, "UL", 2) == 0 ||
           strncmp(id, "NP", 2) == 0;
}

static int is_generic_homebrew_title(const char *name) {
    return name[0] == '\0' ||
           strcmp(name, "Unknown Game") == 0 ||
           strcmp(name, "GAME") == 0;
}

/**
 * @brief Case-insensitive substring search (needle must already be lowercase).
 */
static int path_contains_ci(const char *haystack, const char *needle_lower) {
    if (!haystack || !needle_lower || needle_lower[0] == '\0') {
        return 0;
    }

    for (const char *p = haystack; *p != '\0'; p++) {
        int i = 0;
        while (needle_lower[i] != '\0' && p[i] != '\0') {
            if (ascii_tolower(p[i]) != needle_lower[i]) {
                break;
            }
            i++;
        }
        if (needle_lower[i] == '\0') {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Copies the parent folder of an executable path (EBOOT.PBP → folder name).
 */
static void extract_parent_folder_name(const char *path, char *out, size_t out_size) {
    char path_copy[256];
    char *last_slash;
    char *prev_slash;
    const char *dir_name;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!path || path[0] == '\0') {
        return;
    }

    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    last_slash = strrchr(path_copy, '/');
    if (!last_slash) {
        return;
    }
    *last_slash = '\0';

    prev_slash = strrchr(path_copy, '/');
    dir_name = prev_slash ? prev_slash + 1 : path_copy;
    if (dir_name[0] == '\0') {
        return;
    }

    strncpy(out, dir_name, out_size - 1);
    out[out_size - 1] = '\0';
}

/**
 * @brief Official XMB apps live in PSP/APP (SenseMe, Comic Reader).
 *
 * CFW often launches those with MS_GAME (0x141) instead of MS_APP (0x143), so
 * category stays CAT_HOMEBREW. Detect them from the path / title-ID folder.
 */
static int is_official_app_launch(const char *path, const char *folder_name) {
    return path_contains_ci(path, "/psp/app/") || is_sony_title_id(folder_name);
}

int metadata_is_psp_app(const GameMetadata *metadata) {
    char folder_name[64];

    if (!metadata) {
        return 0;
    }

    /* sceKernelInitApitype MS_APP — CFW may still report CAT_PS1 for 0x143. */
    if (strcmp(metadata->apitype_str, "0x143") == 0) {
        return 1;
    }

    extract_parent_folder_name(metadata->file_path, folder_name, sizeof(folder_name));
    return is_official_app_launch(metadata->file_path, folder_name);
}

/**
 * @brief Reads TITLE and DISC_ID from the SFO embedded in a homebrew EBOOT.PBP.
 *
 * sctrlGetInitPARAM often returns empty strings for homebrews because the kernel
 * does not populate the PARAM cache the same way it does for UMD/ISO titles.
 * This function directly parses the PBP header and embedded SFO, which is the
 * same approach already used for PS1 EBOOTs.
 *
 * Regular homebrew prefers the folder name so dirty SFO titles
 * (e.g. "sm64-port d98e233-dirty") do not leak into the UI. Official apps in
 * PSP/APP (and folders named like a Sony title ID) invert that: the folder is
 * NPIA00013, so TITLE comes from the EBOOT and the Sony DISC_ID is kept.
 */
static void fetch_homebrew_sfo_metadata(GameMetadata *metadata) {
    char folder_name[64];
    int prefer_sfo_title;

    if (metadata->file_path[0] == '\0') {
        debug_log("METADATA", "fetch_homebrew_sfo_metadata: file_path is empty, skipping SFO parse.");
        return;
    }

    extract_parent_folder_name(metadata->file_path, folder_name, sizeof(folder_name));
    prefer_sfo_title = is_official_app_launch(metadata->file_path, folder_name);

    debug_log("METADATA", "fetch_homebrew_sfo_metadata: Parsing homebrew path and SFO from '%s' (folder: '%s', prefer_sfo: %d)",
              metadata->file_path, folder_name, prefer_sfo_title);

    if (prefer_sfo_title) {
        /* SenseMe-style launches: folder is a content ID, not a display name. */
        char title_buf[128];
        title_buf[0] = '\0';
        if (pbp_read_sfo_string(metadata->file_path, "TITLE", title_buf, sizeof(title_buf)) && title_buf[0] != '\0') {
            strncpy(metadata->game_name, title_buf, sizeof(metadata->game_name) - 1);
            metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
            debug_log("METADATA", "fetch_homebrew_sfo_metadata: Using EBOOT TITLE for official-app folder: '%s'", metadata->game_name);
        } else if (is_generic_homebrew_title(metadata->game_name) && folder_name[0] != '\0') {
            strncpy(metadata->game_name, folder_name, sizeof(metadata->game_name) - 1);
            metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
            debug_log("METADATA", "fetch_homebrew_sfo_metadata: EBOOT TITLE missing; using folder name: '%s'", metadata->game_name);
        }
    } else {
        /* First priority: game folder name (users name these cleanly). */
        if (is_generic_homebrew_title(metadata->game_name) && folder_name[0] != '\0') {
            strncpy(metadata->game_name, folder_name, sizeof(metadata->game_name) - 1);
            metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
            debug_log("METADATA", "fetch_homebrew_sfo_metadata: Extracted folder name as TITLE: '%s'", metadata->game_name);
        }

        /* Fallback: TITLE from the SFO if the folder was missing or generic ("GAME"). */
        if (is_generic_homebrew_title(metadata->game_name)) {
            char title_buf[128];
            title_buf[0] = '\0';
            if (pbp_read_sfo_string(metadata->file_path, "TITLE", title_buf, sizeof(title_buf)) && title_buf[0] != '\0') {
                strncpy(metadata->game_name, title_buf, sizeof(metadata->game_name) - 1);
                metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
                debug_log("METADATA", "fetch_homebrew_sfo_metadata: Fallback to SFO TITLE: '%s'", metadata->game_name);
            }
        }
    }

    if (is_sony_title_id(folder_name)) {
        /* Persist the on-disk content ID (NPIA00013) instead of an HBX hash. */
        strncpy(metadata->game_id, folder_name, sizeof(metadata->game_id) - 1);
        metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
        debug_log("METADATA", "fetch_homebrew_sfo_metadata: Using title-ID folder as DISC_ID: '%s'", metadata->game_id);
    } else if (strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
        char id_buf[16];
        id_buf[0] = '\0';
        if (pbp_read_sfo_string(metadata->file_path, "DISC_ID", id_buf, sizeof(id_buf)) && id_buf[0] != '\0') {
            /* Reject lazy SFO copies (LocoRoco UCJS10041) unless this is a real
             * PSP/APP launch, where NP*, UC* or UL* is the authentic content ID. */
            if (prefer_sfo_title || !is_sony_region_prefix(id_buf)) {
                strncpy(metadata->game_id, id_buf, sizeof(metadata->game_id) - 1);
                metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
                debug_log("METADATA", "fetch_homebrew_sfo_metadata: Successfully parsed DISC_ID from SFO: '%s'", metadata->game_id);
            } else {
                debug_log("METADATA", "fetch_homebrew_sfo_metadata: SFO DISC_ID '%s' rejected (Sony region copy).", id_buf);
            }
        }
    }
}

/**
 * @brief Generates a unique ID for Homebrews using a hash of their title.
 *
 * The name is normalized before hashing (spaces stripped, lowercased) so that
 * folder name variations like "GameDiary", "Game Diary" or "gamediary" all
 * produce the same stable HBX identifier across different PSP units.
 */
static void fetch_homebrew_fallback_id(GameMetadata *metadata) {
    if (strcmp(metadata->game_id, "UNKNOWN-00000") == 0) {
        /* Normalize: copy name, strip spaces and lowercase ASCII A-Z.
         * NOTE: Multi-byte UTF-8 sequences (CJK, Hebrew, Cyrillic, etc.) have byte
         * values > 127 (which appear as negative values in signed chars). They bypass
         * this ASCII check and remain unchanged, which is acceptable since case 
         * folding for non-Latin scripts is not required for standard PSP homebrew naming. */
        char normalized[sizeof(metadata->game_name)];
        int out = 0;
        for (int i = 0; metadata->game_name[i] != '\0' && out < (int)sizeof(normalized) - 1; i++) {
            char c = metadata->game_name[i];
            if (c == ' ') continue;                      /* strip spaces */
            if (c >= 'A' && c <= 'Z') c = c + ('a' - 'A'); /* to lowercase */
            normalized[out++] = c;
        }
        normalized[out] = '\0';

        snprintf(metadata->game_id, sizeof(metadata->game_id), "HBX%08X",
                 (unsigned int)hash_string(normalized));
        debug_log("METADATA", "fetch_homebrew_fallback_id: Generated HBX hash ID: '%s' based on normalized TITLE: '%s' (original: '%s')",
                  metadata->game_id, normalized, metadata->game_name);
    }
}

int metadata_fetch(GameMetadata *metadata) {
    int apitype = sceKernelInitApitype();
    metadata->category = apitype_detect_category(apitype);

    snprintf(metadata->apitype_str, sizeof(metadata->apitype_str), "0x%03X", (unsigned int)apitype);
    debug_log("METADATA", "metadata_fetch: Detected APITYPE: %s, Category: %d", metadata->apitype_str, metadata->category);
    set_default_metadata(metadata);

    // Resolve executable path
    memset(metadata->file_path, 0, sizeof(metadata->file_path));
    if (kuKernelInitFileName(metadata->file_path) < 0) {
        metadata->file_path[0] = '\0';
        debug_log("METADATA", "metadata_fetch: kuKernelInitFileName failed to get executable path.");
    } else {
        debug_log("METADATA", "metadata_fetch: Resolved executable path: '%s'", metadata->file_path);
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

        case CAT_HOMEBREW: {
            char folder_name[64];
            int keep_sony_id;

            // For Homebrews, we first try system metadata
            fetch_system_metadata(metadata);

            extract_parent_folder_name(metadata->file_path, folder_name, sizeof(folder_name));
            keep_sony_id = is_official_app_launch(metadata->file_path, folder_name);

            /* ISO drivers spoof DISC_ID to LocoRoco (UCJS10041). Reject UC/UL/NP
             * for real homebrew, but keep them for PSP/APP / title-ID folders. */
            if (!keep_sony_id && is_sony_region_prefix(metadata->game_id)) {
                debug_log("METADATA", "metadata_fetch: Rejected spoofed DISC_ID '%s' for Homebrew.", metadata->game_id);
                strncpy(metadata->game_id, "UNKNOWN-00000", sizeof(metadata->game_id) - 1);
                metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
            }

            /* Resolve name from folder and PBP SFO, then hash if still unknown */
            fetch_homebrew_sfo_metadata(metadata);
            fetch_homebrew_fallback_id(metadata);
            break;
        }

        default:
            // VSH or UNKNOWN
            debug_log("METADATA", "metadata_fetch: Category is VSH or UNKNOWN, skipping deep metadata resolution.");
            break;
    }

    return 1;
}

static int is_disc_launch(const GameMetadata *metadata) {
    return metadata &&
           metadata->file_path[0] != '\0' &&
           strncmp(metadata->file_path, "disc0:/", 7) == 0;
}

static int get_mounted_iso_path(char *out, size_t out_size) {
    if (!out || out_size == 0) return 0;

    out[0] = '\0';
    kuKernelGetUmdFile(out, (int)out_size);
    out[out_size - 1] = '\0';
    return out[0] != '\0';
}

static int get_umd_readiness_state(void) {
    typedef int (*SceUmdGetDriveStatFn)(void);
    static SceUmdGetDriveStatFn get_drive_stat = NULL;
    static int lookup_done = 0;

    if (!lookup_done) {
        // Try all common module and library name combinations for sceUmdGetDriveStat (NID: 0x6B4A146C)
        u32 addr = sctrlHENFindFunction("sceUmd_Driver", "sceUmd_driver", SCE_UMD_GET_DRIVE_STAT_NID);
        if (addr == 0) {
            addr = sctrlHENFindFunction("sceUmd_Driver", "sceUmd", SCE_UMD_GET_DRIVE_STAT_NID);
        }
        if (addr == 0) {
            addr = sctrlHENFindFunction("sceUmd_driver", "sceUmd_driver", SCE_UMD_GET_DRIVE_STAT_NID);
        }
        if (addr == 0) {
            addr = sctrlHENFindFunction("sceUmd_driver", "sceUmd", SCE_UMD_GET_DRIVE_STAT_NID);
        }
        if (addr == 0) {
            addr = sctrlHENFindFunction("Umd_driver", "sceUmd", SCE_UMD_GET_DRIVE_STAT_NID);
        }
        if (addr == 0) {
            addr = sctrlHENFindFunction("sceUmd_Service", "sceUmdUser", SCE_UMD_GET_DRIVE_STAT_NID);
        }
        get_drive_stat = (SceUmdGetDriveStatFn)addr;
        lookup_done = 1;
        debug_log("METADATA", "metadata_fetch_from_umd: sceUmdGetDriveStat lookup -> 0x%X",
                  (unsigned int)addr);
    }

    if (!get_drive_stat) {
        debug_log("METADATA", "metadata_fetch_from_umd: sceUmdGetDriveStat unavailable; skipping disc0 open.");
        return UMD_READINESS_UNAVAILABLE;
    }

    int stat = get_drive_stat();
    if (stat < 0) {
        debug_log("METADATA", "metadata_fetch_from_umd: sceUmdGetDriveStat failed (stat: %d)", stat);
        return UMD_READINESS_NOT_READY;
    }

    debug_log("METADATA", "metadata_fetch_from_umd: UMD drive stat: 0x%X", (unsigned int)stat);
    return (stat & PSP_UMD_READY_FLAG) ? UMD_READINESS_READY : UMD_READINESS_NOT_READY;
}

int metadata_fetch_from_umd(GameMetadata *metadata) {
    char mounted_iso[256];
    int is_virtual_umd = 0;
    static u32 unsafe_fallback_start_ts = 0;

    if (!metadata ||
        (metadata->category != CAT_PSP &&
         !(metadata->category == CAT_UNKNOWN && is_disc_launch(metadata)))) {
        debug_log("METADATA", "metadata_fetch_from_umd: Skipping non-PSP metadata.");
        return 0;
    }

    is_virtual_umd = get_mounted_iso_path(mounted_iso, sizeof(mounted_iso));
    if (is_virtual_umd) {
        debug_log("METADATA", "metadata_fetch_from_umd: Mounted ISO/CSO path: '%s'; allowing virtual disc0 fallback.",
                  mounted_iso);
    }

    if (!is_virtual_umd) {
        int readiness = get_umd_readiness_state();
        if (readiness == UMD_READINESS_NOT_READY) {
            debug_log("METADATA", "metadata_fetch_from_umd: UMD is not READY; skipping disc0 open this attempt.");
            return 0;
        }

        if (readiness == UMD_READINESS_UNAVAILABLE) {
            u32 now = utils_get_timestamp();
            if (unsafe_fallback_start_ts == 0) {
                unsafe_fallback_start_ts = now;
            }

            u32 waited = now - unsafe_fallback_start_ts;
            int fb_count = overlay_notification_get_fb_count();

            // If the game has started active double/triple buffering (fb_count >= 2),
            // it means the display loop is running and it is safe to bypass/shorten the wait.
            // Otherwise, we require the full safety delay.
            u32 required_delay = (fb_count >= 2) ? 10U : PHYSICAL_UMD_UNSAFE_FALLBACK_DELAY_SEC;

            if (waited < required_delay) {
                debug_log("METADATA", "metadata_fetch_from_umd: no READY check (fb_count: %d); waiting %u/%u s before late disc0 fallback.",
                          fb_count, (unsigned int)waited, (unsigned int)required_delay);
                return 0;
            }

            debug_log("METADATA", "metadata_fetch_from_umd: no READY check available (waited %u s, fb_count: %d); allowing physical UMD disc0 fallback.",
                      (unsigned int)waited, fb_count);
        }
    }

    debug_log("METADATA", "metadata_fetch_from_umd: Attempting to read UMD_DATA.BIN...");
    sceKernelDelayThread(200 * 1000); /* 200ms yield before accessing physical UMD */
    SceUID fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0);
    if (fd < 0) {
        debug_log("METADATA", "metadata_fetch_from_umd: Failed to open disc0:/UMD_DATA.BIN (fd: %d)", fd);
        return 0;
    }

    char buf[64];
    memset(buf, 0, sizeof(buf));
    int bytes = sceIoRead(fd, buf, sizeof(buf) - 1);
    sceIoClose(fd);

    if (bytes <= 0) {
        debug_log("METADATA", "metadata_fetch_from_umd: Read empty or failed from UMD_DATA.BIN (%d bytes)", bytes);
        return 0;
    }

    // Parse DISC_ID from "DISC_ID|..." format
    int j = 0;
    for (int i = 0; i < bytes && j < (int)sizeof(metadata->game_id) - 1; i++) {
        if (buf[i] == '|' || buf[i] == '\r' || buf[i] == '\n') break;
        if (buf[i] != '-') {
            metadata->game_id[j++] = buf[i];
        }
    }
    metadata->game_id[j] = '\0';
    if (metadata->game_id[0] == '\0') {
        strncpy(metadata->game_id, "UNKNOWN-00000", sizeof(metadata->game_id) - 1);
        metadata->game_id[sizeof(metadata->game_id) - 1] = '\0';
        debug_log("METADATA", "metadata_fetch_from_umd: UMD_DATA.BIN did not contain a usable DISC_ID.");
        return 0;
    }

    metadata->category = CAT_PSP;
    debug_log("METADATA", "metadata_fetch_from_umd: Parsed UMD DISC_ID: '%s'", metadata->game_id);

    // Try to get Title from UMD SFO
    sceKernelDelayThread(200 * 1000); /* 200ms yield between UMD reads */
    int sfo_read = sfo_read_string("disc0:/PSP_GAME/PARAM.SFO", "TITLE", metadata->game_name, sizeof(metadata->game_name));
    debug_log("METADATA", "metadata_fetch_from_umd: Read TITLE from disc0 SFO (Result: %d) -> '%s'", sfo_read, metadata->game_name);

    return 1;
}

