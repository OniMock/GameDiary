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
static u8 g_icon_enabled = 1;
static u8 g_capture_psp = 1;
static u8 g_capture_ps1 = 1;
static u8 g_capture_homebrew = 1;
static u8 g_capture_psp_app = 1;
static u8 g_capture_unknown = 1;

static void build_path(char *path, size_t size) {
  snprintf(path, size, "%s%s/%s", utils_get_device_prefix(), GDIARY_BASE_DIR, PLUGIN_DAT);
}

static int is_supported_version(u32 version) {
  return version == PLUGIN_CONFIG_VERSION_V1 ||
         version == PLUGIN_CONFIG_VERSION_V2 ||
         version == PLUGIN_CONFIG_VERSION_V3 ||
         version == PLUGIN_CONFIG_VERSION;
}

static u8 clamp_flag(u8 value, u8 fallback) {
  return (value <= 1) ? value : fallback;
}

static void apply_defaults_missing_file(void) {
  g_hotkey_enabled = 0;
  g_icon_enabled = 1;
  g_capture_psp = 1;
  g_capture_ps1 = 1;
  g_capture_homebrew = 1;
  g_capture_psp_app = 1;
  g_capture_unknown = 1;
}

static void apply_file_to_runtime(const PluginConfigFile *file) {
  g_hotkey_enabled = clamp_flag(file->hotkey_enabled, 0);

  if (file->version >= PLUGIN_CONFIG_VERSION_V2) {
    g_icon_enabled = clamp_flag(file->icon_enabled, 1);
  } else {
    g_icon_enabled = 1;
  }

  if (file->version >= PLUGIN_CONFIG_VERSION_V3) {
    g_capture_psp = clamp_flag(file->capture_psp, 1);
    g_capture_ps1 = clamp_flag(file->capture_ps1, 1);
    g_capture_homebrew = clamp_flag(file->capture_homebrew, 1);
    g_capture_psp_app = clamp_flag(file->capture_psp_app, 1);
  } else {
    g_capture_psp = 1;
    g_capture_ps1 = 1;
    g_capture_homebrew = 1;
    g_capture_psp_app = 1;
  }

  if (file->version >= PLUGIN_CONFIG_VERSION) {
    g_capture_unknown = clamp_flag(file->capture_unknown, 1);
  } else {
    g_capture_unknown = 1;
  }
}

void plugin_dat_load(void) {
  apply_defaults_missing_file();

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
    apply_defaults_missing_file();
    return;
  }

  if (!is_supported_version(file.version)) {
    apply_defaults_missing_file();
    return;
  }

  apply_file_to_runtime(&file);
}

int plugin_dat_get_hotkey_enabled(void) {
  return g_hotkey_enabled ? 1 : 0;
}

void plugin_dat_set_hotkey_enabled(int enabled) {
  g_hotkey_enabled = enabled ? 1 : 0;
}

int plugin_dat_get_icon_enabled(void) {
  return g_icon_enabled ? 1 : 0;
}

void plugin_dat_set_icon_enabled(int enabled) {
  g_icon_enabled = enabled ? 1 : 0;
}

int plugin_dat_get_capture_psp(void) {
  return g_capture_psp ? 1 : 0;
}

void plugin_dat_set_capture_psp(int enabled) {
  g_capture_psp = enabled ? 1 : 0;
}

int plugin_dat_get_capture_ps1(void) {
  return g_capture_ps1 ? 1 : 0;
}

void plugin_dat_set_capture_ps1(int enabled) {
  g_capture_ps1 = enabled ? 1 : 0;
}

int plugin_dat_get_capture_homebrew(void) {
  return g_capture_homebrew ? 1 : 0;
}

void plugin_dat_set_capture_homebrew(int enabled) {
  g_capture_homebrew = enabled ? 1 : 0;
}

int plugin_dat_get_capture_psp_app(void) {
  return g_capture_psp_app ? 1 : 0;
}

void plugin_dat_set_capture_psp_app(int enabled) {
  g_capture_psp_app = enabled ? 1 : 0;
}

int plugin_dat_get_capture_unknown(void) {
  return g_capture_unknown ? 1 : 0;
}

void plugin_dat_set_capture_unknown(int enabled) {
  g_capture_unknown = enabled ? 1 : 0;
}

int plugin_dat_save(void) {
  utils_ensure_storage_dirs(utils_get_device_prefix());

  char path[160];
  build_path(path, sizeof(path));

  PluginConfigFile file;
  memset(&file, 0, sizeof(file));
  file.version = PLUGIN_CONFIG_VERSION;
  file.hotkey_enabled = g_hotkey_enabled;
  file.icon_enabled = g_icon_enabled;
  file.capture_psp = g_capture_psp;
  file.capture_ps1 = g_capture_ps1;
  file.capture_homebrew = g_capture_homebrew;
  file.capture_psp_app = g_capture_psp_app;
  file.capture_unknown = g_capture_unknown;

  SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd < 0) {
    return -1;
  }

  int wr = sceIoWrite(fd, &file, sizeof(file));
  sceIoClose(fd);

  return (wr == (int)sizeof(PluginConfigFile)) ? 0 : -2;
}
