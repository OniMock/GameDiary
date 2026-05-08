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
    // Ensure base directory exists
    utils_ensure_storage_dirs(utils_get_device_prefix());
}

void debug_log(const char* module, const char* fmt, ...) {
#ifndef GDIARY_DEBUG
    return;
#endif

    // Get current time for filename
    pspTime ptime;
    sceRtcGetCurrentClock(&ptime, 0);

    char log_path[128];
    snprintf(log_path, sizeof(log_path), "%s" GDIARY_BASE_DIR "/debug-%02d-%02d-%04d.txt", 
             utils_get_device_prefix(), ptime.day, ptime.month, ptime.year);

    SceUID fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) fd = sceIoOpen(log_path, PSP_O_RDWR | PSP_O_CREAT, 0777);

    if (fd >= 0) {
        char msg_buf[512];
        char final_buf[1024];

        va_list args;
        va_start(args, fmt);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
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
        int len = snprintf(final_buf, sizeof(final_buf),
            "[%u] [%s] [%s] %s\r\n",
            (unsigned int)ts, context, module, msg_buf);

        sceIoWrite(fd, final_buf, len);
        sceIoClose(fd);
    }
}
