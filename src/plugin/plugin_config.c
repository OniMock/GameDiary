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
#include "common/db_schema.h"
#include "common/utils.h"
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

static PluginConfigRuntime g_plugin_config = {0};

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

void plugin_config_init(void) {
  /* Sem plugin.dat (ou arquivo inválido): hotkey desligada, tracker.dat ignorado */
  g_plugin_config.hotkey_enabled = 0;

  PluginConfigFile file;
  if (read_config_file(&file) != 0) {
    return;
  }

  if (file.version != PLUGIN_CONFIG_VERSION) {
    return;
  }

  if (file.hotkey_enabled > 1) {
    return;
  }

  g_plugin_config.hotkey_enabled = file.hotkey_enabled;
}

const PluginConfigRuntime *plugin_config_get(void) {
  return &g_plugin_config;
}
