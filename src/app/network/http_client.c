/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/network/http_client.h"
#include "common/debug.h"
#include <curl/curl.h>
#include <pspkernel.h>
#include <string.h>

volatile int g_http_cancel = 0;

#define HTTP_MAX_RETRIES  2
#define HTTP_RETRY_DELAY  (500 * 1000) // 500ms in microseconds
#define HTTP_TIMEOUT_SEC  10

struct curl_memory {
    char *memory;
    size_t size;
    size_t max_size;
};

static size_t curl_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct curl_memory *mem = (struct curl_memory *)userp;

    if (g_http_cancel) return 0; // Abort transfer

    if (mem->size + realsize >= mem->max_size - 1) {
        realsize = mem->max_size - 1 - mem->size;
    }

    if (realsize > 0) {
        memcpy(&(mem->memory[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }

    return size * nmemb; // Return requested size to continue
}

/* Use older progress callback since PSPSDK curl might be older, but match signature */
static int curl_progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    (void)clientp;
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    if (g_http_cancel) return 1; // Non-zero aborts transfer
    return 0;
}

static int http_client_do_request(char* out_buf, size_t buf_size) {
    if (!out_buf || buf_size == 0) return -1;
    out_buf[0] = '\0';

    debug_log("HTTP", "Initializing curl request...");
    CURL *curl = curl_easy_init();
    if (!curl) {
        debug_log("HTTP", "curl_easy_init failed!");
        return -1;
    }

    struct curl_memory chunk;
    chunk.memory = out_buf;
    chunk.size = 0;
    chunk.max_size = buf_size;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, VERSION_ENDPOINT);

        // PSP CA certs are usually missing or very old. Disable SSL verification.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Required for PSP to prevent signals from crashing the app during DNS resolve
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        // PSP requires a custom User-Agent sometimes to pass basic firewalls
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "GameDiary/0.1.1 (PSP)");

#ifdef GDIARY_DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // Progress callback for cancellation
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curl_progress_cb);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        // Timeout and safety
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT_SEC);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }

    debug_log("HTTP", "Sending request... (Timeout: %ds)", HTTP_TIMEOUT_SEC);
    CURLcode res = curl_easy_perform(curl);

    if (g_http_cancel) {
        debug_log("HTTP", "Request cancelled by user.");
        curl_easy_cleanup(curl);
        return -1;
    }

    int ret = -1;
    if (res != CURLE_OK) {
        debug_log("HTTP", "curl_easy_perform failed: %s", curl_easy_strerror(res));
    } else {
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        debug_log("HTTP", "Status Code: %ld", response_code);
        if (response_code == 200) {
            ret = 0;
            debug_log("HTTP", "Read %d bytes.", chunk.size);
        } else {
            debug_log("HTTP", "Non-200 OK status.");
        }
    }

    curl_easy_cleanup(curl);
    return ret;
}

int http_client_fetch_version(char* out_buf, size_t buf_size) {
    g_http_cancel = 0;
    int ret = -1;

    for (int attempt = 0; attempt < HTTP_MAX_RETRIES; attempt++) {
        debug_log("HTTP", "Fetching... Attempt %d of %d", attempt + 1, HTTP_MAX_RETRIES);

        ret = http_client_do_request(out_buf, buf_size);
        if (ret >= 0) break; // Success
        if (g_http_cancel) break; // User aborted

        debug_log("HTTP", "Attempt %d failed. Retrying...", attempt + 1);
        if (attempt < HTTP_MAX_RETRIES - 1) {
            for (int delay = 0; delay < 5 && !g_http_cancel; delay++) { // break down delay so cancellation is fast
                sceKernelDelayThread(HTTP_RETRY_DELAY / 5);
            }
        }
    }

    return ret;
}
