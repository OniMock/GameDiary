/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_NETWORK_MANAGER_H
#define GAMEDIARY_NETWORK_MANAGER_H

/**
 * @file network_manager.h
 * @brief Core PSP network initialization and shutdown.
 * 
 * Provides safe, granular loading/unloading of PRX network modules
 * and initialization of `sceNet`, `sceNetInet`, `sceNetResolver`,
 * `sceNetApctl`, and `sceHttp`.
 */

int network_manager_init(void);
void network_manager_shutdown(void);

#endif // GAMEDIARY_NETWORK_MANAGER_H
