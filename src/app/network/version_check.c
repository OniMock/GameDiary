/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/network/version_check.h"
#include "common/debug.h"
#include <stdio.h>

VersionStatus version_compare(const char *current, const char *remote) {
    if (!current || !remote) return VERSION_PARSE_ERROR;

    int cur_major = 0, cur_minor = 0, cur_patch = 0;
    int rem_major = 0, rem_minor = 0, rem_patch = 0;

    int cur_parsed = sscanf(current, "%d.%d.%d", &cur_major, &cur_minor, &cur_patch);
    int rem_parsed = sscanf(remote, "%d.%d.%d", &rem_major, &rem_minor, &rem_patch);

    if (cur_parsed == 0 || rem_parsed == 0) {
        debug_log("VERSION", "Failed to parse semantic versioning. Cur: %d items, Rem: %d items", cur_parsed, rem_parsed);
        return VERSION_PARSE_ERROR;
    }

    debug_log("VERSION", "Comparing User: %d.%d.%d vs Remote: %d.%d.%d", 
              cur_major, cur_minor, cur_patch, rem_major, rem_minor, rem_patch);

    if (rem_major > cur_major) return VERSION_OUTDATED;
    if (rem_major < cur_major) return VERSION_UP_TO_DATE;

    if (rem_minor > cur_minor) return VERSION_OUTDATED;
    if (rem_minor < cur_minor) return VERSION_UP_TO_DATE;

    if (rem_patch > cur_patch) return VERSION_OUTDATED;

    return VERSION_UP_TO_DATE;
}
