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
 * @file plugin_settings.c
 * @brief Plugin hotkey settings (plugin.dat).
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/ui/ui_loading.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/config/plugin_dat.h"
#include "app/audio/audio_manager.h"
#include <pspctrl.h>
#include <stdbool.h>
#include "common/utils.h"

#define PLUGIN_SETTINGS_ITEM_COUNT 1

static int g_selection = 0;
static float s_anim_hotkey = 0.0f;
static u32 s_loading_start_ms = 0;
static bool s_is_saving = false;
static int s_save_done = 0;

static const char *s_helper_lines[8];
static PopupData s_helper_data;

static void plugin_settings_init(void) {
  plugin_dat_load();

  s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
  s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_CHANGE);
  s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
  s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
  s_helper_lines[4] = "";
  s_helper_lines[5] = i18n_get(MSG_HELP_INFO_LABEL);
  s_helper_lines[6] = i18n_get(MSG_HELP_DESC_PLUGIN_SETTINGS);
  s_helper_lines[7] = "";

  s_helper_data.title = i18n_get(MSG_HELP_TITLE);
  s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
  s_helper_data.lines = s_helper_lines;
  s_helper_data.line_count = 8;
  s_helper_data.show_close_hint = true;

  s_anim_hotkey = plugin_dat_get_hotkey_enabled() ? 1.0f : 0.0f;
  s_is_saving = false;
  s_save_done = 0;
}

static void plugin_settings_update(u32 buttons, u32 pressed) {
  (void)buttons;

  if (pressed & PSP_CTRL_LTRIGGER) {
    popup_open(&s_helper_data);
    return;
  }

  if (s_is_saving) {
    u32 elapsed = utils_get_time_ms() - s_loading_start_ms;
    if (!s_save_done && elapsed > 16) {
      plugin_dat_save();
      s_save_done = 1;
    }
    if (s_save_done && elapsed >= 1000) {
      ui_loading_hide();
      s_is_saving = false;
      s_save_done = 0;
    }
    return;
  }

  if (pressed & PSP_CTRL_UP || pressed & PSP_CTRL_DOWN) {
    g_selection = (g_selection + 1) % PLUGIN_SETTINGS_ITEM_COUNT;
    audio_play_sfx(SFX_NAVIGATE);
  }

  if (pressed & PSP_CTRL_CROSS) {
    if (g_selection == 0) {
      audio_play_sfx(SFX_CONFIRM);
      plugin_dat_set_hotkey_enabled(!plugin_dat_get_hotkey_enabled());
      ui_loading_show(i18n_get(MSG_LOADING));
      s_loading_start_ms = utils_get_time_ms();
      s_is_saving = true;
    }
  }

  if (pressed & PSP_CTRL_CIRCLE) {
    audio_play_sfx(SFX_CANCEL);
    screen_manager_pop();
  }
}

static void plugin_settings_draw(void) {
  renderer_clear(COLOR_BG);

  Rect screen_rect = {0, 0, 480, 272};
  Rect safe_rect = rect_padding(screen_rect, 20);

  ui_draw_title_auto(i18n_get(MSG_SETTINGS_PLUGIN), safe_rect, &GD_IMG_ICON_PLUGIN_32_PNG);

  Rect list_area = {60, 70, 360, 160};

  for (int i = 0; i < PLUGIN_SETTINGS_ITEM_COUNT; i++) {
    Rect item_rect = rect_column(list_area, i, 4, 10);
    const char *label = i18n_get(MSG_SETTINGS_PLUGIN_HOTKEY);

    ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                           label, (i == g_selection), NULL, NULL);

    if (i == 0) {
      bool state = plugin_dat_get_hotkey_enabled() != 0;
      ui_draw_toggle_switch(item_rect.x + item_rect.w - 6, item_rect.y + item_rect.h / 2,
                            state, &s_anim_hotkey);
    }
  }

  ui_draw_standard_hints();
}

Screen g_screen_plugin_settings = {
    plugin_settings_init,
    plugin_settings_update,
    plugin_settings_draw,
    NULL
};
