/**
 * -------------------------------------------------------------
 *  GameDiary Automated Test Suite
 *  Lightweight Zero-Dependency Test Framework
 * -------------------------------------------------------------
 */

#ifndef _TEST_FRAMEWORK_H_
#define _TEST_FRAMEWORK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#endif

/* ANSI Terminal Colors */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BOLD    "\x1b[1m"

typedef struct {
    int total;
    int passed;
    int failed;
    int blocked;
    const char *current_test;
    bool current_test_failed;
} TestContext;

extern TestContext g_test_ctx;

#define TEST_INIT() \
    TestContext g_test_ctx = {0, 0, 0, 0, NULL, false}

#define TEST_SUITE_BEGIN(name) \
    printf("\n" ANSI_COLOR_BOLD ANSI_COLOR_CYAN "=== TEST SUITE: %s ===" ANSI_COLOR_RESET "\n", name)

#define TEST_SUITE_END() \
    do { \
        printf("\n--------------------------------------------------\n"); \
        printf("Summary: " ANSI_COLOR_BOLD "%d" ANSI_COLOR_RESET " tests | ", g_test_ctx.total); \
        printf(ANSI_COLOR_GREEN "%d passed" ANSI_COLOR_RESET " | ", g_test_ctx.passed); \
        if (g_test_ctx.failed > 0) { \
            printf(ANSI_COLOR_RED "%d failed" ANSI_COLOR_RESET " | ", g_test_ctx.failed); \
        } else { \
            printf("0 failed | "); \
        } \
        printf("%d blocked\n", g_test_ctx.blocked); \
        printf("--------------------------------------------------\n\n"); \
    } while (0)

#ifndef _WIN32
#define TEST_RUN(func) \
    do { \
        g_test_ctx.total++; \
        g_test_ctx.current_test = #func; \
        printf("  [RUN]  %s ... ", #func); \
        fflush(stdout); \
        pid_t _pid = fork(); \
        if (_pid < 0) { \
            g_test_ctx.current_test_failed = false; \
            func(); \
            if (!g_test_ctx.current_test_failed) { \
                g_test_ctx.passed++; \
                printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n"); \
            } else { \
                g_test_ctx.failed++; \
                printf(ANSI_COLOR_RED "FAIL" ANSI_COLOR_RESET "\n"); \
            } \
        } else if (_pid == 0) { \
            alarm(5); \
            g_test_ctx.current_test_failed = false; \
            func(); \
            fflush(stdout); \
            exit(g_test_ctx.current_test_failed ? 1 : 0); \
        } else { \
            int _status = 0; \
            waitpid(_pid, &_status, 0); \
            if (WIFEXITED(_status)) { \
                int _code = WEXITSTATUS(_status); \
                if (_code == 0) { \
                    g_test_ctx.passed++; \
                    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n"); \
                } else { \
                    g_test_ctx.failed++; \
                    printf(ANSI_COLOR_RED "FAIL" ANSI_COLOR_RESET "\n"); \
                } \
            } else if (WIFSIGNALED(_status)) { \
                g_test_ctx.failed++; \
                int _sig = WTERMSIG(_status); \
                if (_sig == SIGALRM) { \
                    printf(ANSI_COLOR_RED "TIMEOUT (5s)" ANSI_COLOR_RESET "\n"); \
                } else if (_sig == SIGSEGV) { \
                    printf(ANSI_COLOR_RED "CRASH (SIGSEGV)" ANSI_COLOR_RESET "\n"); \
                } else { \
                    printf(ANSI_COLOR_RED "KILLED (signal %d)" ANSI_COLOR_RESET "\n", _sig); \
                } \
            } else { \
                g_test_ctx.failed++; \
                printf(ANSI_COLOR_RED "FAIL (abnormal termination)" ANSI_COLOR_RESET "\n"); \
            } \
        } \
    } while (0)
#else
#define TEST_RUN(func) \
    do { \
        g_test_ctx.total++; \
        g_test_ctx.current_test = #func; \
        g_test_ctx.current_test_failed = false; \
        printf("  [RUN]  %s ... ", #func); \
        fflush(stdout); \
        func(); \
        if (!g_test_ctx.current_test_failed) { \
            g_test_ctx.passed++; \
            printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n"); \
        } else { \
            g_test_ctx.failed++; \
            printf(ANSI_COLOR_RED "FAIL" ANSI_COLOR_RESET "\n"); \
        } \
    } while (0)
#endif

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("\n    " ANSI_COLOR_RED "ASSERTION FAILED: " #condition ANSI_COLOR_RESET "\n"); \
            printf("    At %s:%d in %s\n", __FILE__, __LINE__, __func__); \
            g_test_ctx.current_test_failed = true; \
            return; \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    ASSERT_TRUE(!(condition))

#define ASSERT_EQ(actual, expected) \
    do { \
        __typeof__(actual) _act = (actual); \
        __typeof__(expected) _exp = (expected); \
        if ((uint64_t)_act != (uint64_t)_exp) { \
            printf("\n    " ANSI_COLOR_RED "ASSERT_EQ FAILED: " #actual " == " #expected ANSI_COLOR_RESET "\n"); \
            printf("    Expected: %lld (0x%llX)\n", (long long)_exp, (unsigned long long)_exp); \
            printf("    Actual:   %lld (0x%llX)\n", (long long)_act, (unsigned long long)_act); \
            printf("    At %s:%d in %s\n", __FILE__, __LINE__, __func__); \
            g_test_ctx.current_test_failed = true; \
            return; \
        } \
    } while (0)

#define ASSERT_NE(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            printf("\n    " ANSI_COLOR_RED "ASSERT_NE FAILED: " #actual " != " #expected ANSI_COLOR_RESET "\n"); \
            printf("    Value was: %lld\n", (long long)(actual)); \
            printf("    At %s:%d in %s\n", __FILE__, __LINE__, __func__); \
            g_test_ctx.current_test_failed = true; \
            return; \
        } \
    } while (0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        const char *_act_s = (actual); \
        const char *_exp_s = (expected); \
        if (!_act_s || !_exp_s || strcmp(_act_s, _exp_s) != 0) { \
            printf("\n    " ANSI_COLOR_RED "ASSERT_STR_EQ FAILED: " #actual " == " #expected ANSI_COLOR_RESET "\n"); \
            printf("    Expected: \"%s\"\n", _exp_s ? _exp_s : "(null)"); \
            printf("    Actual:   \"%s\"\n", _act_s ? _act_s : "(null)"); \
            printf("    At %s:%d in %s\n", __FILE__, __LINE__, __func__); \
            g_test_ctx.current_test_failed = true; \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    ASSERT_TRUE((ptr) != NULL)

#define ASSERT_NULL(ptr) \
    ASSERT_TRUE((ptr) == NULL)

#endif /* _TEST_FRAMEWORK_H_ */
