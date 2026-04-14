#include "app/i18n.h"
#include "common/common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pspkernel.h>

#define MAX_I18N_ENTRIES 256
#define MAX_KEY_LEN     64
#define MAX_VAL_LEN     256

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VAL_LEN];
} i18n_entry;

static i18n_entry *g_entries = NULL;
static int g_entry_count = 0;
static char g_current_lang[8] = "en";
static char g_strings_path[128] = "ms0:/PSP/COMMON/GameDiary/strings";

static void trim(char *str) {
    char *end;
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    *(end + 1) = 0;
}

int i18n_init(const char *lang_code) {
    if (!g_entries) {
        g_entries = (i18n_entry*)malloc(sizeof(i18n_entry) * MAX_I18N_ENTRIES);
        if (!g_entries) return -1;
    }
    return i18n_load(lang_code);
}

int i18n_load(const char *lang_code) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s.lang", g_strings_path, lang_code);

    SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        // Fallback to English if not already trying English
        if (strcmp(lang_code, "en") != 0) {
            return i18n_load("en");
        }
        return -1;
    }

    g_entry_count = 0;
    char line[512];
    int bytes_read;
    char *buf = (char*)malloc(1024 * 16); // 16KB buffer for reading
    if (!buf) {
        sceIoClose(fd);
        return -2;
    }

    bytes_read = sceIoRead(fd, buf, 1024 * 16 - 1);
    sceIoClose(fd);

    if (bytes_read > 0) {
        buf[bytes_read] = 0;
        char *ptr = buf;
        char *next_line;

        while (ptr && *ptr && g_entry_count < MAX_I18N_ENTRIES) {
            next_line = strchr(ptr, '\n');
            if (next_line) *next_line = 0;

            strncpy(line, ptr, sizeof(line) - 1);
            line[sizeof(line) - 1] = 0;

            char *sep = strchr(line, '=');
            if (sep) {
                *sep = 0;
                char *key = line;
                char *val = sep + 1;

                trim(key);
                trim(val);

                if (strlen(key) > 0 && strlen(val) > 0) {
                    strncpy(g_entries[g_entry_count].key, key, MAX_KEY_LEN - 1);
                    strncpy(g_entries[g_entry_count].value, val, MAX_VAL_LEN - 1);
                    g_entry_count++;
                }
            }

            if (next_line) ptr = next_line + 1;
            else ptr = NULL;
        }
    }

    free(buf);
    strncpy(g_current_lang, lang_code, sizeof(g_current_lang) - 1);
    return 0;
}

const char* i18n_get(const char *key) {
    for (int i = 0; i < g_entry_count; i++) {
        if (strcmp(g_entries[i].key, key) == 0) {
            return g_entries[i].value;
        }
    }
    return key; // Fallback to key itself
}

void i18n_cleanup(void) {
    if (g_entries) {
        free(g_entries);
        g_entries = NULL;
    }
    g_entry_count = 0;
}

const char* i18n_current_lang(void) {
    return g_current_lang;
}
