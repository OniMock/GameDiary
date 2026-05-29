/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file plugin_config.h
 * @brief Plugin configuration implementation.
 */

#ifndef _PLUGIN_PLUGIN_CONFIG_H_
#define _PLUGIN_PLUGIN_CONFIG_H_

#include <psptypes.h>

#define PLUGIN_CONFIG_FILE_SIZE  32
#define PLUGIN_CONFIG_VERSION    1

#pragma pack(push, 1)
typedef struct {
  u32 version;
  u8  hotkey_enabled;
  u8  reserved[27];
} PluginConfigFile;
#pragma pack(pop)

typedef struct {
  u8 hotkey_enabled;
} PluginConfigRuntime;

/**
 * @brief Loads plugin.dat into internal cache (call once from module_start).
 * Missing or invalid file → hotkey_enabled = 0 (tracker.dat block list ignored).
 */
void plugin_config_init(void);

/** @brief Returns cached runtime config. */
const PluginConfigRuntime *plugin_config_get(void);

#endif /* _PLUGIN_PLUGIN_CONFIG_H_ */
