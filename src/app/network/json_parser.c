/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/network/json_parser.h"
#include "common/debug.h"
#include <string.h>
#include <stdio.h>

// Helper function for quick naive extraction
static int extract_string(const char* json_buf, const char* key, char* out_buf, size_t max_len) {
    if (!json_buf || !out_buf || max_len == 0) return -1;
    out_buf[0] = '\0';
    
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *pos = strstr(json_buf, search_key);
    if (!pos) return -1;

    pos = strchr(pos, ':');
    if (!pos) return -1;
    
    pos = strchr(pos, '"');
    if (!pos) return -1;
    
    pos++; // skip quote
    size_t i = 0;
    while (*pos != '"' && *pos != '\0' && i < max_len - 1) {
        out_buf[i++] = *pos++;
    }
    out_buf[i] = '\0';
    return (out_buf[0] != '\0') ? 0 : -1;
}

int json_parse_version_info(const char* json_buf, NetworkVersionInfo* out_info) {
    if (!json_buf || !out_info) return -1;
    memset(out_info, 0, sizeof(NetworkVersionInfo));

    if (extract_string(json_buf, "version", out_info->version, sizeof(out_info->version)) < 0) {
        debug_log("JSON", "Version field parsed as empty or missing.");
        return -1;
    }

    extract_string(json_buf, "codename", out_info->codename, sizeof(out_info->codename));
    extract_string(json_buf, "release_date", out_info->release_date, sizeof(out_info->release_date));
    extract_string(json_buf, "url", out_info->url, sizeof(out_info->url));
    
    // Parse mandatory boolean
    out_info->mandatory = 0;
    const char *mand_pos = strstr(json_buf, "\"mandatory\"");
    if (mand_pos) {
        mand_pos = strchr(mand_pos, ':');
        if (mand_pos) {
            if (strstr(mand_pos, "true")) {
                out_info->mandatory = 1;
            }
        }
    }

    debug_log("JSON", "Successfully parsed version: %s, date: %s", out_info->version, out_info->release_date);
    return 0;
}
