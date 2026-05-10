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
#include "cJSON/cJSON.h"
#include <stdio.h>
#include <string.h>

int json_parse_version_info_cjson(const char* json_buf, NetworkVersionInfo* out_info) {
    if (!json_buf || !out_info) return -1;
    memset(out_info, 0, sizeof(NetworkVersionInfo));

    cJSON *root = cJSON_Parse(json_buf);
    if (!root) {
        debug_log("JSON", "Failed to parse version JSON.");
        return -1;
    }

    cJSON *v = cJSON_GetObjectItemCaseSensitive(root, "version");
    if (cJSON_IsString(v) && v->valuestring != NULL) {
        snprintf(out_info->version, sizeof(out_info->version), "%s", v->valuestring);
    } else {
        debug_log("JSON", "Version field missing or invalid.");
        cJSON_Delete(root);
        return -1;
    }

    cJSON *c = cJSON_GetObjectItemCaseSensitive(root, "codename");
    if (cJSON_IsString(c) && c->valuestring != NULL) {
        snprintf(out_info->codename, sizeof(out_info->codename), "%s", c->valuestring);
    }

    cJSON *d = cJSON_GetObjectItemCaseSensitive(root, "release_date");
    if (cJSON_IsString(d) && d->valuestring != NULL) {
        snprintf(out_info->release_date, sizeof(out_info->release_date), "%s", d->valuestring);
    }

    cJSON *u = cJSON_GetObjectItemCaseSensitive(root, "url");
    if (cJSON_IsString(u) && u->valuestring != NULL) {
        snprintf(out_info->url, sizeof(out_info->url), "%s", u->valuestring);
    }

    cJSON *m = cJSON_GetObjectItemCaseSensitive(root, "mandatory");
    if (cJSON_IsBool(m)) {
        out_info->mandatory = cJSON_IsTrue(m);
    }

    cJSON_Delete(root);
    debug_log("JSON", "Successfully parsed version: %s (cJSON)", out_info->version);
    return 0;
}

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
