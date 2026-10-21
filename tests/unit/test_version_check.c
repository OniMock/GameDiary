/**
 * Unit tests for src/app/network/version_check.c
 */

#include "../framework/test_framework.h"
#include "app/network/version_check.h"

static void test_version_compare_logic(void) {
    // Identical versions
    ASSERT_EQ(version_compare("1.0.0", "1.0.0"), VERSION_UP_TO_DATE);
    ASSERT_EQ(version_compare("2.4.15", "2.4.15"), VERSION_UP_TO_DATE);

    // Newer local versions
    ASSERT_EQ(version_compare("1.0.1", "1.0.0"), VERSION_UP_TO_DATE);
    ASSERT_EQ(version_compare("1.1.0", "1.0.9"), VERSION_UP_TO_DATE);
    ASSERT_EQ(version_compare("2.0.0", "1.9.9"), VERSION_UP_TO_DATE);

    // Older local versions (outdated)
    ASSERT_EQ(version_compare("1.0.0", "1.0.1"), VERSION_OUTDATED);
    ASSERT_EQ(version_compare("1.0.9", "1.1.0"), VERSION_OUTDATED);
    ASSERT_EQ(version_compare("1.9.9", "2.0.0"), VERSION_OUTDATED);

    // Missing components (e.g. "1.0" compared to "1.0.1")
    ASSERT_EQ(version_compare("1.0", "1.0.1"), VERSION_OUTDATED);
    ASSERT_EQ(version_compare("1.1", "1.0.5"), VERSION_UP_TO_DATE);

    // Invalid / NULL inputs
    ASSERT_EQ(version_compare(NULL, "1.0.0"), VERSION_PARSE_ERROR);
    ASSERT_EQ(version_compare("1.0.0", NULL), VERSION_PARSE_ERROR);
    ASSERT_EQ(version_compare("invalid", "1.0.0"), VERSION_PARSE_ERROR);
    ASSERT_EQ(version_compare("1.0.0", "invalid"), VERSION_PARSE_ERROR);
}

static void test_json_parse_version(void) {
    const char *valid_json =
        "{\n"
        "  \"version\": \"2.0.0\",\n"
        "  \"codename\": \"Skyfall\",\n"
        "  \"release_date\": \"2026-05-01\",\n"
        "  \"url\": \"https://github.com/OniMock/GameDiary/releases/latest\",\n"
        "  \"mandatory\": true\n"
        "}";

    NetworkVersionInfo info;
    int res = json_parse_version_info_cjson(valid_json, &info);
    ASSERT_EQ(res, 0);
    ASSERT_STR_EQ(info.version, "2.0.0");
    ASSERT_STR_EQ(info.codename, "Skyfall");
    ASSERT_STR_EQ(info.release_date, "2026-05-01");
    ASSERT_STR_EQ(info.url, "https://github.com/OniMock/GameDiary/releases/latest");
    ASSERT_EQ(info.mandatory, 1);

    // Missing version field
    const char *missing_v = "{\"codename\": \"Skyfall\"}";
    res = json_parse_version_info_cjson(missing_v, &info);
    ASSERT_EQ(res, -1);

    // Invalid syntax
    const char *malformed = "{ not a valid json }";
    res = json_parse_version_info_cjson(malformed, &info);
    ASSERT_EQ(res, -1);

    // NULL inputs
    res = json_parse_version_info_cjson(NULL, &info);
    ASSERT_EQ(res, -1);
}

void run_test_version_check(void) {
    TEST_SUITE_BEGIN("Version Check Module");
    TEST_RUN(test_version_compare_logic);
    TEST_RUN(test_json_parse_version);
    TEST_SUITE_END();
}
