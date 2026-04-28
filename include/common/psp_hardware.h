/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef _COMMON_PSP_HARDWARE_H_
#define _COMMON_PSP_HARDWARE_H_

#include <pspkernel.h>

/**
 * @brief Returns PSP model from kernel.
 * @return model id from kuKernelGetModel()
 */
int psp_get_model(void);

/**
 * @brief True if device is PSP-1000 (Fat).
 */
int is_psp_1000(void);

/**
 * @brief True if device is low-end PSP (useful for performance scaling).
 */
int is_psp_low_end(void);

#endif // _COMMON_PSP_HARDWARE_H_
