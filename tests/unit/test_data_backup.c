/**
 * Unit tests for src/app/data/data_backup.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "app/data/data_backup.h"
#include "app/data/data_loader.h"
#include "common/db_schema.h"
#include "common/models.h"
#include "cJSON/cJSON.h"

static const char *k_base_dir = "ms0:/PSP/COMMON/GameDiary";

static void setup_database(void) {
    mock_pspsdk_reset();

    char games_path[256], sessions_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);

    GameRegistryHeader h = {
        .magic = GAMEDIARY_MAGIC,
        .version = DB_VERSION,
        .num_entries = 1,
        .next_uid = 2,
        .ready_flag = GAMEDIARY_MAGIC
    };

    GameEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.uid = 1;
    strcpy(entry.game_id, "ULUS-10041");
    strcpy(entry.game_name, "GTA Liberty City");
    strcpy(entry.apitype_str, "0x120");
    entry.category = CAT_PSP;

    u8 g_buf[sizeof(GameRegistryHeader) + sizeof(GameEntry) + sizeof(GameRegistryHeader)];
    memcpy(g_buf, &h, sizeof(h));
    memcpy(g_buf + sizeof(h), &entry, sizeof(entry));
    memcpy(g_buf + sizeof(h) + sizeof(entry), &h, sizeof(h));
    mock_vfs_map_file(games_path, g_buf, sizeof(g_buf));

    SessionEntry session = { .game_uid = 1, .duration = 3600, .timestamp = 1700000000U };
    mock_vfs_map_file(sessions_path, &session, sizeof(session));
}

static void test_backup_export_import_roundtrip(void) {
    setup_database();
    data_load_all();

    // Export to JSON
    int exp_res = data_backup_export();
    ASSERT_EQ(exp_res, 0);

    // Verify backup.json exists on disk
    char json_path[256];
    snprintf(json_path, sizeof(json_path), "%s%s", k_base_dir, BACKUP_JSON_FILENAME);
    ASSERT_TRUE(mock_vfs_file_exists(json_path));

    char json_buf[2048] = {0};
    size_t rd = mock_vfs_read_file(json_path, json_buf, sizeof(json_buf) - 1);
    ASSERT_TRUE(rd > 0);

    cJSON *root = cJSON_Parse(json_buf);
    ASSERT_NOT_NULL(root);

    cJSON *games = cJSON_GetObjectItem(root, "games");
    ASSERT_TRUE(cJSON_IsArray(games));
    ASSERT_EQ(cJSON_GetArraySize(games), 1);

    cJSON *sessions = cJSON_GetObjectItem(root, "sessions");
    ASSERT_TRUE(cJSON_IsArray(sessions));
    ASSERT_EQ(cJSON_GetArraySize(sessions), 1);
    cJSON_Delete(root);

    data_free();

    // Now test Import: wipe games.dat and sessions.dat from mock disk
    char games_path[256], sessions_path[256];
    snprintf(games_path, sizeof(games_path), "%s/db/%s", k_base_dir, GAMES_DAT);
    snprintf(sessions_path, sizeof(sessions_path), "%s/db/%s", k_base_dir, SESSIONS_DAT);
    sceIoRemove(games_path);
    sceIoRemove(sessions_path);

    int imp_res = data_backup_import();
    ASSERT_EQ(imp_res, 0);

    // Verify files were re-created
    ASSERT_TRUE(mock_vfs_file_exists(games_path));
    ASSERT_TRUE(mock_vfs_file_exists(sessions_path));

    // Reload with data_load_all
    int reload_res = data_load_all();
    ASSERT_EQ(reload_res, 0);
    ASSERT_EQ(data_get_game_count(), 1U);
    ASSERT_EQ(data_get_session_count(), 1U);

    GameStats *g = data_get_games();
    ASSERT_STR_EQ(g[0].entry.game_id, "ULUS-10041");
    ASSERT_STR_EQ(g[0].entry.game_name, "GTA Liberty City");

    SessionEntry *s = data_get_sessions();
    ASSERT_EQ(s[0].duration, 3600U);
    ASSERT_EQ(s[0].timestamp, 1700000000U);

    data_free();
}

static void test_backup_import_errors(void) {
    mock_pspsdk_reset();

    // Import when backup.json does not exist
    ASSERT_EQ(data_backup_import(), -1);

    // Import invalid JSON
    char json_path[256];
    snprintf(json_path, sizeof(json_path), "%s%s", k_base_dir, BACKUP_JSON_FILENAME);
    const char *bad_json = "{ invalid json content ";
    mock_vfs_map_file(json_path, bad_json, strlen(bad_json));
    ASSERT_EQ(data_backup_import(), -4);

    // Import JSON missing arrays
    const char *missing_arrays = "{\"title\": \"no arrays here\"}";
    mock_vfs_map_file(json_path, missing_arrays, strlen(missing_arrays));
    ASSERT_EQ(data_backup_import(), -5);
}

void run_test_data_backup(void) {
    TEST_SUITE_BEGIN("Data Backup (Export/Import) Module");
    TEST_RUN(test_backup_export_import_roundtrip);
    TEST_RUN(test_backup_import_errors);
    TEST_SUITE_END();
}
