/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/network/network_manager.h"
#include "common/debug.h"

#include <pspkernel.h>
#include <psputility.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <curl/curl.h>

#define NET_MEMORY_SIZE  (128 * 1024)
#define NET_THREAD_PRIO  42
#define NET_THREAD_STACK (4 * 1024)
#define HTTP_MEMORY_SIZE (0x25800) // 150KB

static int g_network_initialized = 0;

int network_manager_init(void) {
    if (g_network_initialized) return 0;
    
    int ret = 0;
    debug_log("NET", "Initializing network modules (pkgi-psp style) ...");

    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    
    if ((ret = sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024)) < 0) {
        debug_log("NET", "sceNetInit() failed: 0x%08X", ret);
        return 0; // Notice how we don't abort! If OS is holding the stack, we just use it anyway.
    }

    if ((ret = sceNetInetInit()) < 0) {
        debug_log("NET", "sceNetInetInit() failed: 0x%08X", ret);
        return 0; // Same, continue gracefully
    }

    if ((ret = sceNetApctlInit(0x8000, 48)) < 0) {
        debug_log("NET", "sceNetApctlInit() failed: 0x%08X", ret);
        return 0; // Same, continue gracefully
    }

    curl_global_init(CURL_GLOBAL_ALL);

    g_network_initialized = 1;
    debug_log("NET", "Network modules initialized.");
    
    return 0;
}

void network_manager_shutdown(void) {
    if (!g_network_initialized) return;

    debug_log("NET", "Shutting down network subsystems...");
    
    curl_global_cleanup();

    sceNetApctlTerm();
    sceNetInetTerm();
    sceNetTerm();

    sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
    sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);

    g_network_initialized = 0;
    debug_log("NET", "Network successfully unloaded.");
}
