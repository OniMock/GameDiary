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
 * @brief Plugin settings persisted to plugin.dat.
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_text.h"
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

typedef enum {
  ROW_TOGGLE_HOTKEY = 0,
  ROW_TOGGLE_ICONS,
  ROW_HEADER_CAPTURE,
  ROW_TOGGLE_GAMES,
  ROW_TOGGLE_PS1,
  ROW_TOGGLE_HOMEBREW,
  ROW_TOGGLE_PSP_APP,
  ROW_TOGGLE_UNKNOWN,
  PLUGIN_SETTINGS_ITEM_COUNT
} PluginSettingsRow;

#define MAX_VISIBLE_ITEMS 4

static int g_selection = 0;
static int g_scroll_offset = 0;
static float s_anim[PLUGIN_SETTINGS_ITEM_COUNT];
static u32 s_loading_start_ms = 0;
static bool s_is_saving = false;
static int s_save_done = 0;

static const char *s_helper_lines[8];
static PopupData s_helper_data;

static int row_is_header(int idx) {
  return idx == ROW_HEADER_CAPTURE;
}

static int plugin_settings_get_toggle(int idx) {
  switch (idx) {
    case ROW_TOGGLE_HOTKEY:
      return plugin_dat_get_hotkey_enabled();
    case ROW_TOGGLE_ICONS:
      return plugin_dat_get_icon_enabled();
    case ROW_TOGGLE_GAMES:
      return plugin_dat_get_capture_psp();
    case ROW_TOGGLE_PS1:
      return plugin_dat_get_capture_ps1();
    case ROW_TOGGLE_HOMEBREW:
      return plugin_dat_get_capture_homebrew();
    case ROW_TOGGLE_PSP_APP:
      return plugin_dat_get_capture_psp_app();
    case ROW_TOGGLE_UNKNOWN:
      return plugin_dat_get_capture_unknown();
    default:
      return 0;
  }
}

static void plugin_settings_set_toggle(int idx, int enabled) {
  switch (idx) {
    case ROW_TOGGLE_HOTKEY:
      plugin_dat_set_hotkey_enabled(enabled);
      break;
    case ROW_TOGGLE_ICONS:
      plugin_dat_set_icon_enabled(enabled);
      break;
    case ROW_TOGGLE_GAMES:
      plugin_dat_set_capture_psp(enabled);
      break;
    case ROW_TOGGLE_PS1:
      plugin_dat_set_capture_ps1(enabled);
      break;
    case ROW_TOGGLE_HOMEBREW:
      plugin_dat_set_capture_homebrew(enabled);
      break;
    case ROW_TOGGLE_PSP_APP:
      plugin_dat_set_capture_psp_app(enabled);
      break;
    case ROW_TOGGLE_UNKNOWN:
      plugin_dat_set_capture_unknown(enabled);
      break;
    default:
      break;
  }
}

static const char *plugin_settings_label(int idx) {
  switch (idx) {
    case ROW_TOGGLE_HOTKEY:
      return i18n_get(MSG_SETTINGS_PLUGIN_HOTKEY);
    case ROW_TOGGLE_ICONS:
      return i18n_get(MSG_SETTINGS_PLUGIN_ICONS);
    case ROW_HEADER_CAPTURE:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE);
    case ROW_TOGGLE_GAMES:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE_GAMES);
    case ROW_TOGGLE_PS1:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE_PS1);
    case ROW_TOGGLE_HOMEBREW:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE_HOMEBREW);
    case ROW_TOGGLE_PSP_APP:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE_PSP_APP);
    case ROW_TOGGLE_UNKNOWN:
      return i18n_get(MSG_SETTINGS_PLUGIN_CAPTURE_UNKNOWN);
    default:
      return "";
  }
}

static int step_selection(int dir) {
  int idx = g_selection;
  int i;

  for (i = 0; i < PLUGIN_SETTINGS_ITEM_COUNT; i++) {
    idx = (idx + dir + PLUGIN_SETTINGS_ITEM_COUNT) % PLUGIN_SETTINGS_ITEM_COUNT;
    if (!row_is_header(idx)) {
      return idx;
    }
  }

  return g_selection;
}

static void ensure_selection_visible(void) {
  if (g_selection < g_scroll_offset) {
    g_scroll_offset = g_selection;
  } else if (g_selection >= g_scroll_offset + MAX_VISIBLE_ITEMS) {
    g_scroll_offset = g_selection - (MAX_VISIBLE_ITEMS - 1);
  }
}

static void plugin_settings_init(void) {
  int i;

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

  for (i = 0; i < PLUGIN_SETTINGS_ITEM_COUNT; i++) {
    s_anim[i] = (!row_is_header(i) && plugin_settings_get_toggle(i)) ? 1.0f : 0.0f;
  }

  g_selection = 0;
  g_scroll_offset = 0;
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

  if (pressed & PSP_CTRL_UP) {
    g_selection = step_selection(-1);
    ensure_selection_visible();
    audio_play_sfx(SFX_NAVIGATE);
  }
  if (pressed & PSP_CTRL_DOWN) {
    g_selection = step_selection(1);
    ensure_selection_visible();
    audio_play_sfx(SFX_NAVIGATE);
  }

  if (pressed & PSP_CTRL_CROSS) {
    if (row_is_header(g_selection)) {
      return;
    }
    audio_play_sfx(SFX_CONFIRM);
    plugin_settings_set_toggle(g_selection, !plugin_settings_get_toggle(g_selection));
    ui_loading_show(i18n_get(MSG_LOADING));
    s_loading_start_ms = utils_get_time_ms();
    s_is_saving = true;
  }

  if (pressed & PSP_CTRL_CIRCLE) {
    audio_play_sfx(SFX_CANCEL);
    screen_manager_pop();
  }
}

static void plugin_settings_draw(void) {
  int i;

  renderer_clear(COLOR_BG);

  Rect screen_rect = {0, 0, 480, 272};
  Rect safe_rect = rect_padding(screen_rect, 20);

  ui_draw_title_auto(i18n_get(MSG_SETTINGS_PLUGIN), safe_rect, &GD_IMG_ICON_PLUGIN_32_PNG);

  Rect list_area = {60, 70, 360, 160};

  for (i = 0; i < MAX_VISIBLE_ITEMS; i++) {
    int idx = g_scroll_offset + i;
    const char *label;
    bool selected;
    u32 text_color;
    int text_x;
    int text_h;
    int text_y;
    Rect item_rect;
    Rect text_rect;

    if (idx >= PLUGIN_SETTINGS_ITEM_COUNT) {
      break;
    }

    item_rect = rect_column(list_area, i, MAX_VISIBLE_ITEMS, 6);
    label = plugin_settings_label(idx);
    selected = (idx == g_selection) && !row_is_header(idx);

    if (row_is_header(idx)) {
      text_h = 12;
      text_y = item_rect.y + (item_rect.h - text_h) / 2;
      text_rect = (Rect){item_rect.x + 4, text_y, item_rect.w - 8, text_h};
      ui_draw_text(label, text_rect, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
      continue;
    }

    if (selected) {
      renderer_draw_rect(item_rect.x, item_rect.y, item_rect.w, item_rect.h, COLOR_HIGHLIGHT);
      renderer_draw_rect(item_rect.x, item_rect.y, 3, item_rect.h, COLOR_ACCENT);
    }

    text_color = selected ? COLOR_ACCENT : COLOR_TEXT;
    text_x = item_rect.x + 12;
    text_h = 14;
    text_y = item_rect.y + (item_rect.h - text_h) / 2;
    text_rect = (Rect){text_x, text_y, item_rect.w - (text_x - item_rect.x) - 40, text_h};
    ui_draw_game_name_fixed(label, text_rect, text_color, UI_FONT_SIZE_PRIMARY, ALIGN_LEFT, selected);

    ui_draw_toggle_switch(item_rect.x + item_rect.w - 6, item_rect.y + item_rect.h / 2,
                          plugin_settings_get_toggle(idx) != 0, &s_anim[idx]);
  }

  ui_draw_standard_hints();
}

Screen g_screen_plugin_settings = {
    plugin_settings_init,
    plugin_settings_update,
    plugin_settings_draw,
    NULL
};
