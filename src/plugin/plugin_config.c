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
 * @file plugin_config.c
 * @brief Binary plugin.dat loader (fail-safe defaults).
 */

#include "plugin/plugin_config.h"
#include "plugin/metadata_repository.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

static PluginConfigRuntime g_plugin_config = {0, 1, 1, 1, 1, 1, 1};

static int is_supported_version(u32 version) {
  return version == PLUGIN_CONFIG_VERSION_V1 ||
         version == PLUGIN_CONFIG_VERSION_V2 ||
         version == PLUGIN_CONFIG_VERSION_V3 ||
         version == PLUGIN_CONFIG_VERSION;
}

static u8 clamp_flag(u8 value, u8 fallback) {
  return (value <= 1) ? value : fallback;
}

static void apply_defaults(void) {
  /* Missing/invalid: hotkey off, icons on, capture all on. */
  g_plugin_config.hotkey_enabled = 0;
  g_plugin_config.icon_enabled = 1;
  g_plugin_config.capture_psp = 1;
  g_plugin_config.capture_ps1 = 1;
  g_plugin_config.capture_homebrew = 1;
  g_plugin_config.capture_psp_app = 1;
  g_plugin_config.capture_unknown = 1;
}

static int read_config_file(PluginConfigFile *out) {
  const char *prefix = utils_get_device_prefix();
  char path[160];
  snprintf(path, sizeof(path), "%s%s/%s", prefix, GDIARY_BASE_DIR, PLUGIN_DAT);

  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
  if (fd < 0) {
    return -1;
  }

  PluginConfigFile tmp;
  memset(&tmp, 0, sizeof(tmp));
  int n = sceIoRead(fd, &tmp, sizeof(tmp));
  sceIoClose(fd);

  if (n != (int)sizeof(PluginConfigFile)) {
    return -2;
  }

  *out = tmp;
  return 0;
}

static void apply_file_to_runtime(const PluginConfigFile *file) {
  g_plugin_config.hotkey_enabled = clamp_flag(file->hotkey_enabled, 0);

  /* v1 has no icon field — always enabled. v2+ reads icon_enabled. */
  if (file->version >= PLUGIN_CONFIG_VERSION_V2) {
    g_plugin_config.icon_enabled = clamp_flag(file->icon_enabled, 1);
  } else {
    g_plugin_config.icon_enabled = 1;
  }

  /* v1/v2 have no capture fields — always enabled. v3+ reads the four type flags. */
  if (file->version >= PLUGIN_CONFIG_VERSION_V3) {
    g_plugin_config.capture_psp = clamp_flag(file->capture_psp, 1);
    g_plugin_config.capture_ps1 = clamp_flag(file->capture_ps1, 1);
    g_plugin_config.capture_homebrew = clamp_flag(file->capture_homebrew, 1);
    g_plugin_config.capture_psp_app = clamp_flag(file->capture_psp_app, 1);
  } else {
    g_plugin_config.capture_psp = 1;
    g_plugin_config.capture_ps1 = 1;
    g_plugin_config.capture_homebrew = 1;
    g_plugin_config.capture_psp_app = 1;
  }

  /* v1–v3 have no unknown field — always enabled. v4+ reads capture_unknown. */
  if (file->version >= PLUGIN_CONFIG_VERSION) {
    g_plugin_config.capture_unknown = clamp_flag(file->capture_unknown, 1);
  } else {
    g_plugin_config.capture_unknown = 1;
  }
}

void plugin_config_init(void) {
  apply_defaults();

  PluginConfigFile file;
  if (read_config_file(&file) != 0) {
    return;
  }

  if (!is_supported_version(file.version)) {
    return;
  }

  apply_file_to_runtime(&file);
}

const PluginConfigRuntime *plugin_config_get(void) {
  return &g_plugin_config;
}

int plugin_config_should_capture(const GameMetadata *meta) {
  if (!meta) {
    return 1;
  }

  /* Official PSP/APP content is stored as CAT_HOMEBREW (or CAT_PS1 for 0x143). */
  if (metadata_is_psp_app(meta)) {
    return g_plugin_config.capture_psp_app ? 1 : 0;
  }

  switch (meta->category) {
    case CAT_PSP:
      return g_plugin_config.capture_psp ? 1 : 0;
    case CAT_PS1:
      return g_plugin_config.capture_ps1 ? 1 : 0;
    case CAT_HOMEBREW:
      return g_plugin_config.capture_homebrew ? 1 : 0;
    case CAT_UNKNOWN:
    default:
      return g_plugin_config.capture_unknown ? 1 : 0;
  }
}
