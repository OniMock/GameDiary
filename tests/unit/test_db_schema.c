/**
 * Unit tests for struct alignment, packed sizes, and constants in db_schema.h
 */

#include "../framework/test_framework.h"
#include "common/db_schema.h"
#include "plugin/plugin_config.h"
#include "plugin/tracker_db.h"

static void test_struct_sizes_and_alignment(void) {
    // Binary database structures must have exact fixed sizes across compilers
    ASSERT_EQ(sizeof(GameRegistryHeader), 32U);
    ASSERT_EQ(sizeof(GameEntry), 88U);
    ASSERT_EQ(sizeof(SessionEntry), 12U);

    // Config file sizes
    ASSERT_EQ(sizeof(PluginConfigFile), 32U);

    // Tracker file sizes
    ASSERT_EQ(sizeof(TrackerHeader), 16U);
    ASSERT_EQ(sizeof(TrackerEntry), 16U);
}

static void test_schema_constants(void) {
    ASSERT_EQ(GAMEDIARY_MAGIC, 0x444D4147U); // 'GAMD' in little-endian
    ASSERT_EQ(TRACKER_MAGIC, 0x4B415254U);   // 'TRAK' in little-endian
    ASSERT_EQ(DB_VERSION, 3U);
    ASSERT_EQ(PLUGIN_CONFIG_VERSION, 4U);
    ASSERT_EQ(TRACKER_DB_VERSION, 1U);
    ASSERT_STR_EQ(GDIARY_SELF_ID, "HBX0E9162F8");
}

void run_test_db_schema(void) {
    TEST_SUITE_BEGIN("Database Schema & Binary Layout Module");
    TEST_RUN(test_struct_sizes_and_alignment);
    TEST_RUN(test_schema_constants);
    TEST_SUITE_END();
}
