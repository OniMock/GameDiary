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
#include "common/models.h"

#define PLUGIN_CONFIG_FILE_SIZE  32
#define PLUGIN_CONFIG_VERSION    4
#define PLUGIN_CONFIG_VERSION_V1 1
#define PLUGIN_CONFIG_VERSION_V2 2
#define PLUGIN_CONFIG_VERSION_V3 3

#pragma pack(push, 1)
typedef struct {
    u32 version;
    u8  hotkey_enabled;
    u8  icon_enabled;
    u8  capture_psp;
    u8  capture_ps1;
    u8  capture_homebrew;
    u8  capture_psp_app;
    u8  capture_unknown;
    u8  reserved[21];
} PluginConfigFile;
#pragma pack(pop)

_Static_assert(sizeof(PluginConfigFile) == PLUGIN_CONFIG_FILE_SIZE, "plugin.dat must stay 32 bytes");

typedef struct {
  u8 hotkey_enabled;
  u8 icon_enabled;
  u8 capture_psp;
  u8 capture_ps1;
  u8 capture_homebrew;
  u8 capture_psp_app;
  u8 capture_unknown;
} PluginConfigRuntime;

/**
 * @brief Loads plugin.dat into internal cache (call once from module_start).
 * Missing or invalid file → hotkey off, icons on, all capture flags on.
 * v1 → icon on (field absent). v1/v2 → capture flags on (fields absent).
 * v3 → capture_unknown on (field absent).
 */
void plugin_config_init(void);

/** @brief Returns cached runtime config. */
const PluginConfigRuntime *plugin_config_get(void);

/**
 * @brief True when the plugin should start tracking this boot's title.
 * PSP/APP is detected from path/apitype and does not change persisted category.
 */
int plugin_config_should_capture(const GameMetadata *meta);

#endif /* _PLUGIN_PLUGIN_CONFIG_H_ */
