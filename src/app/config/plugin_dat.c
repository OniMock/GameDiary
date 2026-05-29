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
 * @file plugin_dat.c
 * @brief Application persistence for plugin.dat (shared format with PRX).
 */

#include "app/config/plugin_dat.h"
#include "plugin/plugin_config.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

static u8 g_hotkey_enabled = 0;

static void build_path(char *path, size_t size) {
  snprintf(path, size, "%s%s/%s", utils_get_device_prefix(), GDIARY_BASE_DIR, PLUGIN_DAT);
}

void plugin_dat_load(void) {
  g_hotkey_enabled = 0;

  char path[160];
  build_path(path, sizeof(path));

  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if (fd < 0) {
    return;
  }

  PluginConfigFile file;
  memset(&file, 0, sizeof(file));
  int n = sceIoRead(fd, &file, sizeof(file));
  sceIoClose(fd);

  if (n != (int)sizeof(PluginConfigFile)) {
    return;
  }
  if (file.version != PLUGIN_CONFIG_VERSION) {
    return;
  }
  if (file.hotkey_enabled > 1) {
    return;
  }

  g_hotkey_enabled = file.hotkey_enabled;
}

int plugin_dat_get_hotkey_enabled(void) {
  return g_hotkey_enabled ? 1 : 0;
}

void plugin_dat_set_hotkey_enabled(int enabled) {
  g_hotkey_enabled = enabled ? 1 : 0;
}

int plugin_dat_save(void) {
  utils_ensure_storage_dirs(utils_get_device_prefix());

  char path[160];
  build_path(path, sizeof(path));

  PluginConfigFile file;
  memset(&file, 0, sizeof(file));
  file.version = PLUGIN_CONFIG_VERSION;
  file.hotkey_enabled = g_hotkey_enabled;

  SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd < 0) {
    return -1;
  }

  int wr = sceIoWrite(fd, &file, sizeof(file));
  sceIoClose(fd);

  return (wr == (int)sizeof(PluginConfigFile)) ? 0 : -2;
}
