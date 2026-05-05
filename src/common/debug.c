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

    char log_path[128];
    snprintf(log_path, sizeof(log_path), "%s" GDIARY_BASE_DIR "/debug.txt", utils_get_device_prefix());

    SceUID fd = sceIoOpen(log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) fd = sceIoOpen(log_path, PSP_O_RDWR | PSP_O_CREAT, 0777);

    if (fd >= 0) {
        char msg_buf[512];
        char final_buf[1024];

        va_list args;
        va_start(args, fmt);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
        va_end(args);

        u32 ts = utils_get_timestamp();

        int len = snprintf(final_buf, sizeof(final_buf),
            "[%u] [%s] %s\r\n",
            (unsigned int)ts, module, msg_buf);

        sceIoWrite(fd, final_buf, len);
        sceIoClose(fd);
    }
}
