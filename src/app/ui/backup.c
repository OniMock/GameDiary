/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_popup.h"
#include "app/ui/ui_loading.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/audio/audio_manager.h"
#include "app/data/data_backup.h"
#include "app/data/data_loader.h"
#include "common/utils.h"
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>

#include "app/ui/ui_style.h"

#define BACKUP_MENU_COUNT 2

static int g_selection = 0;
static bool s_confirm_import = false;

static u32 s_loading_start_ms = 0;
static bool s_is_changing = false;
static int s_pending_action = -1; // 1: Export, 2: Import

static const char* s_result_lines[2];
static PopupData s_result_popup;

static void show_result(MessageId msg_id) {
    s_result_lines[0] = i18n_get(msg_id);
    s_result_lines[1] = "";
    s_result_popup.title = i18n_get(MSG_SETTINGS_BACKUP);
    s_result_popup.icon = &GD_IMG_ICON_BACKUP_32_PNG;
    s_result_popup.lines = s_result_lines;
    s_result_popup.line_count = 1;
    s_result_popup.show_close_hint = true;
    popup_open(&s_result_popup);
}

static const char* s_helper_lines[8];
static PopupData s_helper_data;

static void backup_init(void) {
    g_selection = 0;
    s_confirm_import = false;
    s_is_changing = false;
    s_pending_action = -1;

    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_X_SELECT);
    s_helper_lines[2] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[3] = i18n_get(MSG_HELP_BTN_ARROWS_NAVIGATE);
    s_helper_lines[4] = i18n_get(MSG_HELP_BTN_START_MENU);
    s_helper_lines[5] = "";
    s_helper_lines[6] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[7] = i18n_get(MSG_HELP_DESC_BACKUP);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 8;
    s_helper_data.show_close_hint = true;
}

static int s_operation_result = 0;
static int s_active_action = 0; // 1: Export, 2: Import

static void backup_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    if (s_is_changing) {
        u32 elapsed = utils_get_time_ms() - s_loading_start_ms;

        // Step 1: Perform the work on the first frame after show
        if (s_pending_action != -1 && elapsed > 16) {
            s_active_action = s_pending_action;
            s_pending_action = -1; // Work done marker

            if (s_active_action == 1) { // Export
                s_operation_result = data_backup_export();
            } else if (s_active_action == 2) { // Import
                s_operation_result = data_backup_import();
                if (s_operation_result == 0) {
                    data_free();
                    data_load_all();
                }
            }
        }

        // Step 2: Wait for minimum duration (1000ms) and show result
        if (s_pending_action == -1 && elapsed >= 1000) {
            ui_loading_hide();
            s_is_changing = false;
            s_confirm_import = false; // Finally hide the confirm overlay if it was there

            if (s_active_action == 1) { // Export
                if (s_operation_result == 0) show_result(MSG_BACKUP_EXPORT_OK);
                else show_result(MSG_BACKUP_ERROR);
            } else if (s_active_action == 2) { // Import
                if (s_operation_result == 0) show_result(MSG_BACKUP_IMPORT_OK);
                else if (s_operation_result == -1) show_result(MSG_BACKUP_NOT_FOUND);
                else show_result(MSG_BACKUP_ERROR);
            }
        }
        return;
    }

    if (s_confirm_import) {
        if (pressed & PSP_CTRL_CROSS) {
            audio_play_sfx(SFX_CONFIRM);
            // Don't set s_confirm_import = false yet!
            
            ui_loading_show(i18n_get(MSG_LOADING));
            s_loading_start_ms = utils_get_time_ms();
            s_is_changing = true;
            s_pending_action = 2; // Import
        } else if (pressed & PSP_CTRL_CIRCLE) {
            audio_play_sfx(SFX_CANCEL);
            s_confirm_import = false;
        }
        return;
    }

    if (pressed & PSP_CTRL_UP) {
        g_selection = (g_selection - 1 + BACKUP_MENU_COUNT) % BACKUP_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }
    if (pressed & PSP_CTRL_DOWN) {
        g_selection = (g_selection + 1) % BACKUP_MENU_COUNT;
        audio_play_sfx(SFX_NAVIGATE);
    }

    if (pressed & PSP_CTRL_CROSS) {
        audio_play_sfx(SFX_CONFIRM);
        if (g_selection == 0) {
            // Export
            ui_loading_show(i18n_get(MSG_LOADING));
            s_loading_start_ms = utils_get_time_ms();
            s_is_changing = true;
            s_pending_action = 1; // Export
        } else if (g_selection == 1) {
            // Import
            s_confirm_import = true;
        }
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        audio_play_sfx(SFX_CANCEL);
        screen_manager_pop();
    }
}

static void backup_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    ui_draw_title_auto(i18n_get(MSG_SETTINGS_BACKUP), safe_rect, &GD_IMG_ICON_BACKUP_32_PNG);

    if (s_confirm_import) {
        Rect card_rect = {20, 80, 440, 140};
        ui_draw_card(card_rect, COLOR_CARD, COLOR_BORDER);
        
        // 1. Draw "WARNING" title in red
        Rect title_rect = {card_rect.x, card_rect.y + 10, card_rect.w, 30};
        ui_draw_text(i18n_get(MSG_WARNING), title_rect, COLOR_DANGER, UI_FONT_SIZE_TITLE_HUGE, ALIGN_CENTER);
        
        // 2. Draw a thin separator line
        int sep_y = card_rect.y + 40;
        renderer_draw_rect(card_rect.x + 20, sep_y, card_rect.w - 40, 1, COLOR_BORDER);
        
        // 3. Draw the message body
        const char *warn_text = i18n_get(MSG_BACKUP_IMPORT_WARN);
        char line1[128] = {0};
        char line2[128] = {0};
        
        const char *nl = strchr(warn_text, '\n');
        if (nl) {
            int len = nl - warn_text;
            if (len >= (int)sizeof(line1)) len = (int)sizeof(line1) - 1;
            strncpy(line1, warn_text, len);
            snprintf(line2, sizeof(line2), "%s", nl + 1);
        } else {
            snprintf(line1, sizeof(line1), "%s", warn_text);
        }

        Rect text_rect_1 = {card_rect.x, sep_y + 15, card_rect.w, 20};
        Rect text_rect_2 = {card_rect.x, sep_y + 35, card_rect.w, 20};
        
        ui_draw_text(line1, text_rect_1, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_CENTER);
        if (line2[0] != '\0') {
            ui_draw_text(line2, text_rect_2, COLOR_TEXT, UI_FONT_SIZE_PRIMARY, ALIGN_CENTER);
        }

        if (!s_is_changing) {
            ui_draw_hint_footer(i18n_get(MSG_HELP_BTN_X_CONFIRM), 20, COLOR_SUBTEXT);
            ui_draw_hint_footer(i18n_get(MSG_HELP_BTN_O_BACK), 160, COLOR_SUBTEXT);
        }
        return;
    }

    Rect list_area = {60, 70, 360, 160};

    for (int i = 0; i < BACKUP_MENU_COUNT; i++) {
        Rect item_rect = rect_column(list_area, i, 4, 10);

        const char* label = "";
        if (i == 0) {
            label = i18n_get(MSG_BACKUP_EXPORT);
        } else if (i == 1) {
            label = i18n_get(MSG_BACKUP_IMPORT);
        }

        ui_draw_menu_item_auto(item_rect.x, item_rect.y, item_rect.w, item_rect.h,
                         label, (i == g_selection), NULL, NULL);
    }

    if (!s_is_changing) {
        ui_draw_standard_hints();
    }
}

Screen g_screen_backup = {
    backup_init,
    backup_update,
    backup_draw,
    NULL
};
