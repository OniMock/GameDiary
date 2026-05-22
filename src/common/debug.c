/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "common/debug.h"
#include "common/utils.h"
#include "common/db_schema.h"
#include <pspkernel.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void debug_init(void) {
#ifdef GDIARY_DEBUG
    // Ensure base directory exists
    utils_ensure_storage_dirs(utils_get_device_prefix());
#endif
}

#ifdef GDIARY_PLUGIN
// Minimal implementation of itoa with padding support for the plugin
static char* mini_utoa(unsigned int value, char* str, char* end, int width,
                       unsigned int base, int uppercase) {
    char temp[32];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    if (base < 2 || base > 16) base = 10;
    if (value == 0) temp[i++] = '0';
    else {
        while (value > 0) {
            temp[i++] = digits[value % base];
            value /= base;
        }
    }
    while (i < width && i < (int)sizeof(temp) - 1) temp[i++] = '0';
    while (i > 0 && str < end) *str++ = temp[--i];
    return str;
}

static char* mini_itoa(int value, char* str, char* end, int width) {
    if (value < 0) {
        unsigned int magnitude = (unsigned int)(-(value + 1)) + 1;
        if (str < end) *str++ = '-';
        return mini_utoa(magnitude, str, end, width, 10, 0);
    }
    return mini_utoa((unsigned int)value, str, end, width, 10, 0);
}

static int mini_vsnprintf(char* buffer, size_t n, const char* format, va_list arg) {
    if (n == 0) return 0;

    char* p = buffer;
    char* end = buffer + n - 1;
    const char* f = format;

    while (*f && p < end) {
        if (*f == '%') {
            f++;
            int width = 0;
            if (*f == '0') {
                f++;
                while (*f >= '0' && *f <= '9') {
                    width = width * 10 + (*f - '0');
                    f++;
                }
            }
            while (*f == 'l') f++;

            if (*f == 's') {
                char* s = va_arg(arg, char*);
                if (!s) s = "(null)";
                while (*s && p < end) *p++ = *s++;
            } else if (*f == 'd') {
                int val = va_arg(arg, int);
                p = mini_itoa(val, p, end, width);
            } else if (*f == 'u') {
                unsigned int val = va_arg(arg, unsigned int);
                p = mini_utoa(val, p, end, width, 10, 0);
            } else if (*f == 'x' || *f == 'X') {
                unsigned int val = va_arg(arg, unsigned int);
                p = mini_utoa(val, p, end, width, 16, *f == 'X');
            } else if (*f == 'c') {
                int val = va_arg(arg, int);
                if (p < end) *p++ = (char)val;
            } else if (*f == '%') {
                *p++ = '%';
            } else if (*f != '\0') {
                if (p < end) *p++ = '%';
                if (p < end) *p++ = *f;
            }
        } else {
            *p++ = *f;
        }
        f++;
    }
    *p = '\0';
    return p - buffer;
}

static int mini_snprintf(char* buffer, size_t n, const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    int ret = mini_vsnprintf(buffer, n, format, arg);
    va_end(arg);
    return ret;
}

#define _snprintf mini_snprintf
#define _vsnprintf mini_vsnprintf
#else
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif

void debug_log(const char* module, const char* fmt, ...) {
#ifndef GDIARY_DEBUG
    return;
#endif

    // Get current time for filename
    ScePspDateTime ptime;
    sceRtcGetCurrentClock(&ptime, 0);

    char log_path[128];
    _snprintf(log_path, sizeof(log_path), "%s" GDIARY_BASE_DIR "/debug-%02d-%02d-%04d.txt", 
             utils_get_device_prefix(), ptime.day, ptime.month, ptime.year);

    SceUID fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) fd = sceIoOpen(log_path, PSP_O_RDWR | PSP_O_CREAT, 0777);

    if (fd >= 0) {
        char msg_buf[512];
        char final_buf[1024];

        va_list args;
        va_start(args, fmt);
        _vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
        va_end(args);

        // Use the standard timestamp for log content
        u32 ts = utils_get_timestamp();

        const char* context = "UNKNOWN";
#if defined(GDIARY_APP)
        context = "APP";
#elif defined(GDIARY_PLUGIN)
        context = "PLUGIN";
#endif

        // [Timestamp] [CONTEXT] [MODULE] Message
        int len = _snprintf(final_buf, sizeof(final_buf),
            "[%u] [%s] [%s] %s\r\n",
            (unsigned int)ts, context, module, msg_buf);

        sceIoWrite(fd, final_buf, len);
        sceIoClose(fd);
    }
}
