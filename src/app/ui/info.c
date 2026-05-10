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
  * @file info.c
  * @brief Info screen implementation (About & Support).
  */

  #include "app/ui/screen.h"
  #include "app/ui/ui_components.h"
  #include "app/ui/ui_layout.h"
  #include "app/ui/ui_popup.h"
  #include "app/i18n/i18n.h"
  #include "app/render/renderer.h"
  #include "app/render/font.h"
  #include "app/audio/audio_manager.h"
  #include <pspctrl.h>
  #include <stdio.h>
  #include <string.h>

  #include "app/network/network_manager.h"
  #include "app/network/http_client.h"
  #include "app/network/version_check.h"
  #include "app/ui/ui_loading.h"
  #include "app/render/renderer.h"
  #include "common/debug.h"
  #include <psputility.h>
  #include <psputility_modules.h>
  #include <pspnet_apctl.h>
  #include <pspwlan.h>
  #include <pspgu.h>
  #include <pspdisplay.h>
  #include <time.h>


  #define INFO_MENU_COUNT 3

  static int g_selection = 0;

  static const char* s_helper_lines[8];
  static PopupData s_helper_data;

  static void info_draw(void); // Forward declaration

  static void info_init(void) {
      s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
      s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_SELECT);
      s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
      s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
      s_helper_lines[4] = i18n_get(MSG_HELP_BTN_START_MENU);
      s_helper_lines[5] = "";
      s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
      s_helper_lines[7] = i18n_get(MSG_HELP_DESC_INFO);

      s_helper_data.title = i18n_get(MSG_HELP_TITLE);
      s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
      s_helper_data.lines = s_helper_lines;
      s_helper_data.line_count = 8;
      s_helper_data.show_close_hint = true;
  }

 static const char* s_version_result_lines[5];
static PopupData s_version_result_data;

static char s_formatted_msg_line[128];
static char s_formatted_version_line[128];
static char s_formatted_date_line[64];
static char s_formatted_site_line[320];

/**
 * @brief Formats an ISO date string to the user's locale.
 *
 * @param iso_date The date in ISO format (YYYY-MM-DD).
 * @param out_buf The buffer to store the formatted date.
 * @param max_len The maximum size of the buffer.
 */
static void format_i18n_date(const char* iso_date, char* out_buf, size_t max_len) {
    if (!iso_date || !out_buf || max_len == 0) return;

    int year, month, day;
    if (sscanf(iso_date, "%d-%d-%d", &year, &month, &day) == 3) {
        struct tm dt = {0};
        dt.tm_year = year - 1900;
        dt.tm_mon  = month - 1;
        dt.tm_mday = day;

        strftime(out_buf, max_len, i18n_get(MSG_DATE_FORMAT), &dt);
    } else {
        snprintf(out_buf, max_len, "%s", iso_date);
    }
}

/**
 * @brief Shows a popup with the version information.
 *
 * @param msg_id The message ID to display.
 * @param info The version information.
 */
static void show_version_popup_advanced(MessageId msg_id, NetworkVersionInfo* info) {
    if (info && info->version[0] != '\0') {
        char date_str[32];
        format_i18n_date(info->release_date, date_str, sizeof(date_str));

        snprintf(s_formatted_msg_line, sizeof(s_formatted_msg_line), "%s", i18n_get(msg_id));

        if (info->codename[0] != '\0') {
            snprintf(s_formatted_version_line, sizeof(s_formatted_version_line), "%s: %s (%s)", 
                     i18n_get(MSG_ABOUT_VERSION), info->version, info->codename);
        } else {
            snprintf(s_formatted_version_line, sizeof(s_formatted_version_line), "%s: %s", 
                     i18n_get(MSG_ABOUT_VERSION), info->version);
        }

        snprintf(s_formatted_date_line, sizeof(s_formatted_date_line), "%s: %s", i18n_get(MSG_ABOUT_DATE), date_str);
        snprintf(s_formatted_site_line, sizeof(s_formatted_site_line), "%s: %s", i18n_get(MSG_SITE_LABEL), info->url);

        s_version_result_lines[0] = s_formatted_msg_line;
        s_version_result_lines[1] = s_formatted_version_line;
        s_version_result_lines[2] = s_formatted_date_line;
        
        if (msg_id == MSG_INFO_VERSION_LATEST) {
            // Up to date: Show Version, Codename, Date. Site not needed.
            s_version_result_data.line_count = 3;
        } else {
            // Update available: Show all including Site URL
            s_version_result_lines[3] = s_formatted_site_line;
            s_version_result_data.line_count = 4;
        }
    } else {
        snprintf(s_formatted_msg_line, sizeof(s_formatted_msg_line), "%s", i18n_get(msg_id));
        s_version_result_lines[0] = s_formatted_msg_line;
        s_version_result_data.line_count = 1;
    }

    s_version_result_data.title = i18n_get(MSG_INFO_CHECK_VERSION);
    s_version_result_data.icon = &GD_IMG_ICON_CHECK_VERSION_32_PNG;
    s_version_result_data.lines = s_version_result_lines;
    s_version_result_data.show_close_hint = true;

    popup_open(&s_version_result_data);
}

volatile int g_http_done = 0;
int g_http_ret = 0;
char g_http_json_buf[1024];

/**
 * @brief Worker thread for fetching version information.
 *
 * @param args The number of arguments.
 * @param argp The arguments.
 * @return 0 on success, negative value on error.
 */
static int http_worker_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    g_http_ret = http_client_fetch_version(g_http_json_buf, sizeof(g_http_json_buf));
    g_http_done = 1;
    return 0;
}

/**
 * @brief Checks for a new version of the application.
 *
 * This function fetches the version information from the endpoint,
 * and displays the result in a popup.
 */
void info_check_version_flow(void) {
    ui_loading_show(i18n_get(MSG_INFO_CHECK_VERSION));

    // Finish current frame that main.c started so we can run Utility securely outside GU List
    renderer_end_frame();

    debug_log("NET", "WLAN switch: %d", sceWlanGetSwitchState());
    debug_log("NET", "WLAN power: %d", sceWlanDevIsPowerOn());
    int apstate = 0;
    sceNetApctlGetState(&apstate);
    debug_log("NET", "APCtl state: %d", apstate);

    if (network_manager_init() < 0) {
        ui_loading_hide();
        // Restore GU before returning and showing popup
        renderer_start_frame();
        show_version_popup_advanced(MSG_INFO_VERSION_ERROR, NULL);
        network_manager_shutdown();
        return;
    }
    ui_loading_hide();

    static __attribute__((aligned(16))) pspUtilityNetconfData data;
    memset(&data, 0, sizeof(data));
    data.base.size = sizeof(pspUtilityNetconfData);

    // Pega o idioma do sistema em vez de hardcodar
    int lang = 0;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &lang);
    data.base.language = lang;

    int buttons = 0;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &buttons);
    data.base.buttonSwap = buttons;

    struct pspUtilityNetconfAdhoc adhocparam;
    memset(&adhocparam, 0, sizeof(adhocparam));

    data.base.graphicsThread = 0x11;
    data.base.accessThread   = 0x13;
    data.base.fontThread     = 0x12;
    data.base.soundThread    = 0x10;
    data.action              = 3; // PSP_NETCONF_ACTION_CONNECTAP
    data.hotspot             = 0;
    data.adhocparam          = &adhocparam;

    // The utility is implicitly loaded on 6.xx!
    ui_loading_show(i18n_get(MSG_INFO_CHECK_VERSION));

    int init_res = sceUtilityNetconfInitStart(&data);
    debug_log("NET", "sceUtilityNetconfInitStart returned 0x%08X", init_res);

    int done = 0;

    while(!done) {
        renderer_start_frame();
        info_draw();
        sceGuFinish();
        sceGuSync(0, 0);

        switch(sceUtilityNetconfGetStatus()) {
            case PSP_UTILITY_DIALOG_NONE:
                done = 1;
                break;

            case PSP_UTILITY_DIALOG_VISIBLE:
                sceUtilityNetconfUpdate(1);
                break;

            case PSP_UTILITY_DIALOG_QUIT:
                sceUtilityNetconfShutdownStart();
                done = 1;
                break;

            case PSP_UTILITY_DIALOG_FINISHED:
                done = 1;
                break;

            default:
                break;
        }

        sceDisplayWaitVblankStart();
        renderer_swap_buffers();
    }

    int state = 0; // PSP_NET_APCTL_STATE_DISCONNECTED
    sceNetApctlGetState(&state);

    if (state != 4) { // PSP_NET_APCTL_STATE_GOT_IP
        debug_log("NET", "No IP after netconf wait, final state: %d", state);
        ui_loading_hide(); // Must hide the loading screen if we cancel!
        network_manager_shutdown();
        return;
    }

    ui_loading_show(i18n_get(MSG_INFO_CHECK_VERSION));

    g_http_done = 0;
    g_http_ret = -1;
    memset(g_http_json_buf, 0, sizeof(g_http_json_buf));

    SceUID thid = sceKernelCreateThread("http_worker", http_worker_thread, 0x20, 0x10000, 0, 0);
    if (thid >= 0) {
        debug_log("HTTP", "Worker thread created (ID: 0x%08X)", thid);
        sceKernelStartThread(thid, 0, 0);
    } else {
        debug_log("HTTP", "Thread creation failed (0x%08X). Running fallback...", thid);
        g_http_ret = http_client_fetch_version(g_http_json_buf, sizeof(g_http_json_buf));
        g_http_done = 1;
    }

    while (!g_http_done) {
        renderer_start_frame();
        info_draw();
        ui_loading_update();
        ui_loading_render();
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        renderer_swap_buffers();
        
        /* Yield execution to other threads (like the http_worker)
         * to prevent the main loop from starving the network task. */
        sceKernelDelayThread(0);
    }

    /* Release the kernel thread object now that it has finished.
     * sceKernelCreateThread allocates a KID slot that persists until
     * explicitly deleted — skipping this leaks a thread handle. */
    if (thid >= 0) {
        debug_log("HTTP", "Cleaning up thread ID: 0x%08X", thid);
        sceKernelDeleteThread(thid);
    } else {
        debug_log("HTTP", "No thread cleanup needed (fallback was used)");
    }

    ui_loading_hide();

    if (g_http_ret < 0) {
        show_version_popup_advanced(MSG_INFO_VERSION_CONN_ERROR, NULL);
    } else {
        // Evaluate the JSON.
        NetworkVersionInfo out_info;
        int parse_ret = json_parse_version_info_cjson(g_http_json_buf, &out_info);

        if (parse_ret < 0) {
            debug_log("HTTP", "Failed to parse JSON!");
            show_version_popup_advanced(MSG_INFO_VERSION_PARSE_ERROR, NULL);
        } else {
            debug_log("HTTP", "JSON parsed correctly! Version string: %s", out_info.version);

#ifdef APP_VERSION
            const char* local_version = APP_VERSION;
#else
            const char* local_version = "0.0.0";
#endif

            VersionStatus vs = version_compare(local_version, out_info.version);

            if (vs == VERSION_UP_TO_DATE) {
                // Version is the same
                show_version_popup_advanced(MSG_INFO_VERSION_LATEST, &out_info);
            } else if (vs == VERSION_OUTDATED) {
                // Version is newer on server
                show_version_popup_advanced(MSG_INFO_VERSION_AVAILABLE, &out_info);
            } else {
                // Unexpected comparison result
                show_version_popup_advanced(MSG_INFO_VERSION_ERROR, NULL);
            }
        }
    }

    network_manager_shutdown();
}

  static void info_update(u32 buttons, u32 pressed) {
      (void)buttons;

      if (pressed & PSP_CTRL_LTRIGGER) {
          popup_open(&s_helper_data);
          return;
      }

      if (pressed & PSP_CTRL_UP) {
          g_selection = (g_selection - 1 + INFO_MENU_COUNT) % INFO_MENU_COUNT;
          audio_play_sfx(SFX_NAVIGATE);
      }
      if (pressed & PSP_CTRL_DOWN) {
          g_selection = (g_selection + 1) % INFO_MENU_COUNT;
          audio_play_sfx(SFX_NAVIGATE);
      }

      if (pressed & PSP_CTRL_CROSS) {
          if (g_selection == 0) {
              audio_play_sfx(SFX_CONFIRM);
              screen_manager_push(&g_screen_about);
          } else if (g_selection == 1) {
              audio_play_sfx(SFX_CONFIRM);
              screen_manager_push(&g_screen_support);
          } else if (g_selection == 2) {
              audio_play_sfx(SFX_CONFIRM);
              info_check_version_flow();
          }
      }

      if (pressed & PSP_CTRL_CIRCLE) {
          audio_play_sfx(SFX_CANCEL);
          screen_manager_pop();
      }
  }

  static void info_draw(void) {
      renderer_clear(COLOR_BG);

      Rect screen_rect = {0, 0, 480, 272};
      Rect safe_rect = rect_padding(screen_rect, 20);

      ui_draw_title_auto(i18n_get(MSG_MENU_INFO), safe_rect, &GD_IMG_ICON_INFO_32_PNG);

      Rect list_area = {60, 70, 360, 160};

      for (int i = 0; i < INFO_MENU_COUNT; i++) {
          Rect item_rect = rect_column(list_area, i, 4, 10);

          const char* label = "";
          const ImageResource* left_icon = NULL;

          if (i == 0) {
              label = i18n_get(MSG_SETTINGS_ABOUT);
              left_icon = &GD_IMG_ICON_ABOUT_32_PNG;
          } else if (i == 1) {
              label = i18n_get(MSG_SETTINGS_SUPPORT);
              left_icon = &GD_IMG_ICON_SUPPORT_32_PNG;
          } else if (i == 2) {
              label = i18n_get(MSG_INFO_CHECK_VERSION);
              left_icon = &GD_IMG_ICON_CHECK_VERSION_32_PNG;
          }

          ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                          label, (i == g_selection), left_icon, NULL);
      }

      ui_draw_standard_hints();
  }

  Screen g_screen_info = {
      info_init,
      info_update,
      info_draw,
      NULL
  };
