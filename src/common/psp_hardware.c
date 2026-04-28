/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "common/psp_hardware.h"
#include <pspkernel.h>

/**
 * Internal cached RAM check result
 */
static int s_is_low_end_cached = -1;

/**
 * @brief Detect low-end PSP using available RAM.
 * @return 1 if PSP-1000, 0 otherwise
 */
static int detect_low_end(void) {
    if (s_is_low_end_cached < 0) {
        s_is_low_end_cached =
            (sceKernelTotalFreeMemSize() < (34 * 1024 * 1024));
    }
    return s_is_low_end_cached;
}

/**
 * @brief Get PSP model (emulated via RAM heuristic).
 * @return 0 = PSP-1000 (low-end), 1 = others
 */
int psp_get_model(void) {
    return detect_low_end() ? 0 : 1;
}

/**
 * @brief Check if PSP is PSP-1000.
 * @return 1 if PSP-1000, 0 otherwise
 */
int is_psp_1000(void) {
    return detect_low_end();
}

/**
 * @brief Check if PSP is low-end.
 * @return 1 if PSP is low-end, 0 otherwise
 */
int is_psp_low_end(void) {
    return detect_low_end();
}
