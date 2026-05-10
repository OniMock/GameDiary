/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_VERSION_CHECK_H
#define GAMEDIARY_VERSION_CHECK_H

/**
 * @file version_check.h
 * @brief Compares semantic versioning strings.
 */

typedef enum {
    VERSION_UP_TO_DATE,
    VERSION_OUTDATED,
    VERSION_PARSE_ERROR
} VersionStatus;

typedef struct {
    char version[32];
    char codename[64];
    char release_date[32];
    char url[256];
    int mandatory;
} NetworkVersionInfo;

/**
 * @brief Semantically compares the remote version against the current local version.
 * 
 * @param current The current version string (e.g., "1.0.0").
 * @param remote The remote version string to compare against (e.g., "1.1.0").
 * @return VERSION_UP_TO_DATE if current >= remote, VERSION_OUTDATED if current < remote, VERSION_PARSE_ERROR on fail.
 */
VersionStatus version_compare(const char *current, const char *remote);

/**
 * @brief Parses version, date, url, and mandatory flags from a JSON payload using cJSON.
 * 
 * @param json_buf The null-terminated JSON buffer.
 * @param out_info Pointer to the struct holding the parsed info.
 * @return 0 on success, -1 on failure.
 */
int json_parse_version_info_cjson(const char* json_buf, NetworkVersionInfo* out_info);

#endif // GAMEDIARY_VERSION_CHECK_H
