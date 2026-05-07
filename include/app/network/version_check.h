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

/**
 * @brief Semantically compares the remote version against the current local version.
 * 
 * @param current The current version string (e.g., "1.0.0").
 * @param remote The remote version string to compare against (e.g., "1.1.0").
 * @return VERSION_UP_TO_DATE if current >= remote, VERSION_OUTDATED if current < remote, VERSION_PARSE_ERROR on fail.
 */
VersionStatus version_compare(const char *current, const char *remote);

#endif // GAMEDIARY_VERSION_CHECK_H
