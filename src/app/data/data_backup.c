/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/data/data_backup.h"
#include "app/data/data_loader.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include "common/debug.h"
#include "cJSON/cJSON.h"

#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static u32 calculate_generic_checksum(const void *data, size_t len, size_t skip_offset) {
    u32 hash = 2166136261U; // FNV offset basis
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        if (i >= skip_offset && i < skip_offset + sizeof(u32)) continue;
        hash ^= p[i];
        hash *= 16777619U; // FNV prime
    }
    return hash;
}

int data_backup_export(void) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return -1;

    // Games
    cJSON *games_arr = cJSON_AddArrayToObject(root, "games");
    u32 game_count = data_get_game_count();
    GameStats *games = data_get_games();

    for (u32 i = 0; i < game_count; i++) {
        cJSON *g = cJSON_CreateObject();
        cJSON_AddStringToObject(g, "game_id", games[i].entry.game_id);
        cJSON_AddStringToObject(g, "name", games[i].entry.game_name);
        cJSON_AddStringToObject(g, "apitype", games[i].entry.apitype_str);
        cJSON_AddNumberToObject(g, "category", games[i].entry.category);
        cJSON_AddItemToArray(games_arr, g);
    }

    // Sessions
    cJSON *sessions_arr = cJSON_AddArrayToObject(root, "sessions");
    u32 session_count = data_get_session_count();
    SessionEntry *sessions = data_get_sessions();

    for (u32 i = 0; i < session_count; i++) {
        // Find game_id from uid
        const char *game_id = "UNKNOWN";
        for (u32 j = 0; j < game_count; j++) {
            if (games[j].entry.uid == sessions[i].game_uid) {
                game_id = games[j].entry.game_id;
                break;
            }
        }

        cJSON *s = cJSON_CreateObject();
        cJSON_AddStringToObject(s, "game_id", game_id);
        cJSON_AddNumberToObject(s, "duration", sessions[i].duration);
        cJSON_AddNumberToObject(s, "timestamp", sessions[i].timestamp);
        cJSON_AddItemToArray(sessions_arr, s);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) return -2;

    char backup_path[256];
    snprintf(backup_path, sizeof(backup_path), "%s%s%s", utils_get_device_prefix(), GDIARY_BASE_DIR, BACKUP_JSON_FILENAME);

    SceUID fd = sceIoOpen(backup_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd < 0) {
        free(json_str);
        return -3;
    }

    sceIoWrite(fd, json_str, strlen(json_str));
    sceIoClose(fd);
    free(json_str);

    return 0;
}

// Helper struct for mapping game_id -> new_uid during import
typedef struct {
    char game_id[16];
    u32 uid;
} UidMap;

int data_backup_import(void) {
    char backup_path[256];
    snprintf(backup_path, sizeof(backup_path), "%s%s%s", utils_get_device_prefix(), GDIARY_BASE_DIR, BACKUP_JSON_FILENAME);

    SceUID fd = sceIoOpen(backup_path, PSP_O_RDONLY, 0777);
    if (fd < 0) return -1; // File not found

    SceOff size = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);

    char *json_str = (char*)malloc(size + 1);
    if (!json_str) {
        sceIoClose(fd);
        return -2;
    }

    if (sceIoRead(fd, json_str, size) != size) {
        free(json_str);
        sceIoClose(fd);
        return -3;
    }
    json_str[size] = '\0';
    sceIoClose(fd);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return -4;

    cJSON *games_arr = cJSON_GetObjectItemCaseSensitive(root, "games");
    cJSON *sessions_arr = cJSON_GetObjectItemCaseSensitive(root, "sessions");

    if (!cJSON_IsArray(games_arr) || !cJSON_IsArray(sessions_arr)) {
        cJSON_Delete(root);
        return -5;
    }

    int game_count = cJSON_GetArraySize(games_arr);
    UidMap *map = NULL;
    if (game_count > 0) {
        map = (UidMap*)malloc(sizeof(UidMap) * game_count);
    }

    const char *prefix = utils_get_device_prefix();
    char games_path[256];
    snprintf(games_path, sizeof(games_path), "%s%s/%s/%s", prefix, GDIARY_BASE_DIR, GDIARY_DB_DIR, GAMES_DAT);
    char sessions_path[256];
    snprintf(sessions_path, sizeof(sessions_path), "%s%s/%s/%s", prefix, GDIARY_BASE_DIR, GDIARY_DB_DIR, SESSIONS_DAT);

    // Write games.dat
    SceUID g_fd = sceIoOpen(games_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (g_fd < 0) {
        if (map) free(map);
        cJSON_Delete(root);
        return -6;
    }

    GameRegistryHeader h;
    memset(&h, 0, sizeof(GameRegistryHeader));
    h.magic = GAMEDIARY_MAGIC;
    h.version = DB_VERSION;
    h.num_entries = 0;
    h.next_uid = 1;

    // Leave space for primary header
    sceIoLseek(g_fd, sizeof(GameRegistryHeader), PSP_SEEK_SET);

    cJSON *g_item;
    int idx = 0;
    cJSON_ArrayForEach(g_item, games_arr) {
        cJSON *j_id = cJSON_GetObjectItemCaseSensitive(g_item, "game_id");
        cJSON *j_name = cJSON_GetObjectItemCaseSensitive(g_item, "name");
        cJSON *j_api = cJSON_GetObjectItemCaseSensitive(g_item, "apitype");
        cJSON *j_cat = cJSON_GetObjectItemCaseSensitive(g_item, "category");

        if (cJSON_IsString(j_id) && j_id->valuestring != NULL) {
            GameEntry entry;
            memset(&entry, 0, sizeof(GameEntry));
            entry.uid = h.next_uid++;
            
            snprintf(entry.game_id, sizeof(entry.game_id), "%s", j_id->valuestring);
            if (cJSON_IsString(j_name)) snprintf(entry.game_name, sizeof(entry.game_name), "%s", j_name->valuestring);
            if (cJSON_IsString(j_api)) snprintf(entry.apitype_str, sizeof(entry.apitype_str), "%s", j_api->valuestring);
            if (cJSON_IsNumber(j_cat)) entry.category = j_cat->valueint;

            sceIoWrite(g_fd, &entry, sizeof(GameEntry));
            h.num_entries++;

            if (map && idx < game_count) {
                snprintf(map[idx].game_id, sizeof(map[idx].game_id), "%s", entry.game_id);
                map[idx].uid = entry.uid;
                idx++;
            }
        }
    }

    // Finalize header
    h.ready_flag = GAMEDIARY_MAGIC;
    h.checksum = calculate_generic_checksum(&h, 32, offsetof(GameRegistryHeader, checksum));

    // Write primary header
    sceIoLseek(g_fd, 0, PSP_SEEK_SET);
    sceIoWrite(g_fd, &h, sizeof(GameRegistryHeader));

    // Write backup header at end
    sceIoLseek(g_fd, 0, PSP_SEEK_END);
    sceIoWrite(g_fd, &h, sizeof(GameRegistryHeader));

    sceIoClose(g_fd);

    // Write sessions.dat
    SceUID s_fd = sceIoOpen(sessions_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (s_fd >= 0) {
        cJSON *s_item;
        cJSON_ArrayForEach(s_item, sessions_arr) {
            cJSON *j_id = cJSON_GetObjectItemCaseSensitive(s_item, "game_id");
            cJSON *j_dur = cJSON_GetObjectItemCaseSensitive(s_item, "duration");
            cJSON *j_ts = cJSON_GetObjectItemCaseSensitive(s_item, "timestamp");

            if (cJSON_IsString(j_id) && cJSON_IsNumber(j_dur) && cJSON_IsNumber(j_ts)) {
                u32 session_uid = 0;
                if (map) {
                    for (int k = 0; k < game_count; k++) {
                        if (strcmp(map[k].game_id, j_id->valuestring) == 0) {
                            session_uid = map[k].uid;
                            break;
                        }
                    }
                }

                if (session_uid > 0) {
                    SessionEntry entry;
                    entry.game_uid = session_uid;
                    entry.duration = j_dur->valueint;
                    entry.timestamp = j_ts->valueint;
                    sceIoWrite(s_fd, &entry, sizeof(SessionEntry));
                }
            }
        }
        sceIoClose(s_fd);
    }

    if (map) free(map);
    cJSON_Delete(root);

    return 0;
}
