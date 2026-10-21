/**
 * -------------------------------------------------------------
 *  GameDiary Automated Test Runner
 *  Master test execution entry point.
 * -------------------------------------------------------------
 */

#include "framework/test_framework.h"
#include "framework/mock_pspsdk.h"

TEST_INIT();

/* Unit Test Suite Prototypes */
void run_test_db_schema(void);
void run_test_utils(void);
void run_test_sfo_parser(void);
void run_test_apitype(void);
void run_test_plugin_config(void);
void run_test_tracker_db(void);
void run_test_tracker_runtime(void);
void run_test_input_combo(void);
void run_test_storage(void);
void run_test_app_config(void);
void run_test_app_plugin_dat(void);
void run_test_game_category(void);
void run_test_version_check(void);
void run_test_carousel_state(void);
void run_test_data_loader(void);
void run_test_data_backup(void);
void run_test_stats_calculator(void);
void run_test_i18n(void);
void run_test_metadata_repository(void);
void run_test_overlay_blit(void);
void run_test_tracker_time_accounting(void);

/* Integration Test Suite Prototypes */
void run_test_integration_tracker_pipeline(void);
void run_test_integration_storage_recovery(void);
void run_test_integration_data_lifecycle(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("\n" ANSI_COLOR_BOLD ANSI_COLOR_BLUE);
    printf("========================================================\n");
    printf("          GAMEDIARY AUTOMATED TEST SUITE                \n");
    printf("========================================================\n");
    printf(ANSI_COLOR_RESET);

    // Initialize mock sandbox
    mock_pspsdk_init("/tmp/gamediary_sandbox");

    /* Unit Tests */
    run_test_db_schema();
    run_test_utils();
    run_test_sfo_parser();
    run_test_apitype();
    run_test_plugin_config();
    run_test_tracker_db();
    run_test_tracker_runtime();
    run_test_input_combo();
    run_test_storage();
    run_test_app_config();
    run_test_app_plugin_dat();
    run_test_game_category();
    run_test_version_check();
    run_test_carousel_state();
    run_test_data_loader();
    run_test_data_backup();
    run_test_stats_calculator();
    run_test_i18n();
    run_test_metadata_repository();
    run_test_overlay_blit();
    run_test_tracker_time_accounting();

    /* Integration Tests */
    run_test_integration_tracker_pipeline();
    run_test_integration_storage_recovery();
    run_test_integration_data_lifecycle();

    printf("\n" ANSI_COLOR_BOLD);
    printf("========================================================\n");
    printf("                  FINAL TEST RESULTS                    \n");
    printf("========================================================\n" ANSI_COLOR_RESET);
    printf("Total Tests Executed: " ANSI_COLOR_BOLD "%d" ANSI_COLOR_RESET "\n", g_test_ctx.total);
    printf("Passed:               " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n", g_test_ctx.passed);
    if (g_test_ctx.failed > 0) {
        printf("Failed:               " ANSI_COLOR_RED "%d" ANSI_COLOR_RESET "\n", g_test_ctx.failed);
    } else {
        printf("Failed:               0\n");
    }
    printf("Blocked:              %d\n", g_test_ctx.blocked);
    printf("========================================================\n\n");

    return (g_test_ctx.failed > 0) ? 1 : 0;
}
