/**
 * Unit tests for src/plugin/metadata_repository.c
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/metadata_repository.h"
#include "common/models.h"
#include "common/utils.h"

static void test_metadata_is_psp_app(void) {
    // 1. apitype 0x143
    GameMetadata m1 = {0};
    strncpy(m1.apitype_str, "0x143", sizeof(m1.apitype_str) - 1);
    ASSERT_EQ(metadata_is_psp_app(&m1), 1);

    // 2. path containing /psp/app/ (case insensitive)
    GameMetadata m2 = {
        .apitype_str = "0x141",
        .file_path = "ms0:/PSP/APP/NPIA00013/EBOOT.PBP"
    };
    ASSERT_EQ(metadata_is_psp_app(&m2), 1);

    GameMetadata m2_ci = {
        .apitype_str = "0x141",
        .file_path = "ms0:/psp/app/ComicReader/eboot.pbp"
    };
    ASSERT_EQ(metadata_is_psp_app(&m2_ci), 1);

    // 3. folder named like a Sony title ID (e.g. NPIA00013)
    GameMetadata m3 = {
        .apitype_str = "0x141",
        .file_path = "ms0:/PSP/GAME/NPIA00013/EBOOT.PBP"
    };
    ASSERT_EQ(metadata_is_psp_app(&m3), 1);

    // 4. Standard homebrew game in /PSP/GAME/
    GameMetadata m4 = {
        .apitype_str = "0x141",
        .file_path = "ms0:/PSP/GAME/Doom-PSP/EBOOT.PBP"
    };
    ASSERT_EQ(metadata_is_psp_app(&m4), 0);

    // NULL metadata
    ASSERT_EQ(metadata_is_psp_app(NULL), 0);
}

static void test_homebrew_fallback_id_normalization(void) {
    // Test the normalization rule:
    // Strip spaces and lowercase ASCII A-Z, then djb2 hash.
    // "gamediary" -> 0x0E9162F8
    const char *names[] = {
        "gamediary",
        "GameDiary",
        "Game Diary",
        "GAME DIARY",
        "   G a m e D i a r y   "
    };

    for (size_t i = 0; i < sizeof(names)/sizeof(names[0]); i++) {
        char normalized[64];
        int out = 0;
        for (int j = 0; names[i][j] != '\0' && out < (int)sizeof(normalized) - 1; j++) {
            char c = names[i][j];
            if (c == ' ') continue;
            if (c >= 'A' && c <= 'Z') c = c + ('a' - 'A');
            normalized[out++] = c;
        }
        normalized[out] = '\0';

        char generated_id[16];
        snprintf(generated_id, sizeof(generated_id), "HBX%08X", (unsigned int)hash_string(normalized));
        ASSERT_STR_EQ(generated_id, "HBX0E9162F8");
    }
}

void run_test_metadata_repository(void) {
    TEST_SUITE_BEGIN("Metadata Repository Module");
    TEST_RUN(test_metadata_is_psp_app);
    TEST_RUN(test_homebrew_fallback_id_normalization);
    TEST_SUITE_END();
}
