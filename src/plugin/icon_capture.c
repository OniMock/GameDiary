#include "plugin/icon_capture.h"
#include "common/utils.h"
#include "common/models.h"
#include <pspkernel.h>
#include <pspsdk/kubridge.h>
#include <stdio.h>
#include <string.h>

void utils_capture_icon(const char *game_id, u8 category, const char *dest_dir, const char *executable_path) {
  char dest_path[256];
  snprintf(dest_path, sizeof(dest_path), "%s/%s.png", dest_dir, game_id);

  if (category == CAT_PSP) {
    if (utils_copy_file("disc0:/PSP_GAME/ICON0.PNG", dest_path) == 0) {
      return;
    }
  }

  if (executable_path && executable_path[0] != '\0') {
    int ext_res = utils_extract_pbp_icon(executable_path, dest_path);
    if (ext_res == 0) return;

    char sidecar_path[256];
    strncpy(sidecar_path, executable_path, sizeof(sidecar_path));
    sidecar_path[sizeof(sidecar_path) - 1] = '\0';
    char *last_slash = strrchr(sidecar_path, '/');
    if (last_slash) {
      strcpy(last_slash + 1, "ICON0.PNG");
      int sc1 = utils_copy_file(sidecar_path, dest_path);
      if (sc1 == 0) return;

      strcpy(last_slash + 1, "icon0.png");
      int sc2 = utils_copy_file(sidecar_path, dest_path);
      if (sc2 == 0) return;
    }
  }
}
