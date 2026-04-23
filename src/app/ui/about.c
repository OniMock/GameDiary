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
#include "app/ui/ui_text.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include <pspctrl.h>
#include <stdio.h>
#include <time.h>

// Fallbacks for localized build info passed via Makefile
#ifndef APP_VERSION
    #define APP_VERSION "0.0.1-dev"
#endif
#ifndef SDK_VERSION
    #define SDK_VERSION "Unknown"
#endif

#ifndef BUILD_YEAR
    #define BUILD_YEAR 2026
#endif
#ifndef BUILD_MONTH
    #define BUILD_MONTH 4
#endif
#ifndef BUILD_DAY
    #define BUILD_DAY 22
#endif

static const char* s_helper_lines[5];
static PopupData s_helper_data;

static void about_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[2] = "";
    s_helper_lines[3] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[4] = i18n_get(MSG_HELP_DESC_ABOUT);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 5;
    s_helper_data.show_close_hint = true;
}

static void about_update(u32 buttons, u32 pressed) {
    (void)buttons;

    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
        return;
    }

    if (pressed & PSP_CTRL_CIRCLE) {
        screen_manager_pop();
    }
}

static void about_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    // Header
    ui_draw_title_auto(i18n_get(MSG_SETTINGS_ABOUT), safe_rect, &GD_IMG_ICON_ABOUT_32_PNG);

    int content_x = safe_rect.x + 10;
    int content_y = safe_rect.y + 60;
    int content_w = safe_rect.w - 20;

    int card2_w = 85;
    int gap = 10;
    int card1_w = content_w - card2_w - gap;

    // --- Card 1: App Info (Left) ---
    Rect card1_rect = { content_x, content_y, card1_w, 145 };
    ui_draw_card(card1_rect, COLOR_CARD, COLOR_BORDER);

    int tx = card1_rect.x + 15;
    int ty = card1_rect.y + 12;
    int tw = card1_rect.w - 30;

    // App Name & Developer
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), i18n_get(MSG_ABOUT_TITLE), APP_TITLE);
    ui_draw_text(title_buf, (Rect){tx, ty, tw, 20}, COLOR_ACCENT, UI_FONT_SIZE_PRIMARY, ALIGN_LEFT);
    ty += 22;
    ui_draw_text(i18n_get(MSG_ABOUT_DEVELOPER), (Rect){tx, ty, tw, 14}, COLOR_SUBTEXT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
    ty += 24;

    char desc_buf[512];
    snprintf(desc_buf, sizeof(desc_buf), i18n_get(MSG_ABOUT_DESC), APP_TITLE);

    char wrapped[6][MAX_LINE_WIDTH];
    int line_count = 0;
    ui_text_wrap(desc_buf, UI_FONT_SIZE_NORMAL, tw, wrapped, 6, &line_count);

    for (int i = 0; i < line_count; i++) {
        ui_draw_text(wrapped[i], (Rect){tx, ty, tw, 14}, COLOR_TEXT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
        ty += 16;
    }
    ty += 8;

    // GitHub Link
    char github_buf[128];
    snprintf(github_buf, sizeof(github_buf), i18n_get(MSG_ABOUT_GITHUB), APP_TITLE);
    ui_draw_text(github_buf, (Rect){tx, ty, tw, 14}, COLOR_ACCENT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);

    // --- Card 2: Build Info (Right) ---
    Rect card2_rect = { content_x + card1_w + gap, content_y, card2_w, 145 };
    ui_draw_card(card2_rect, COLOR_CARD, COLOR_BORDER);

    int ctx = card2_rect.x + 8;
    int cty = card2_rect.y + 12;
    int ctw = card2_rect.w - 16;

    // Version
    ui_draw_text(i18n_get(MSG_ABOUT_VERSION), (Rect){ctx, cty, ctw, 14}, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    cty += 14;
    ui_draw_text(APP_VERSION, (Rect){ctx, cty, ctw, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
    cty += 25;

    // SDK
    ui_draw_text(i18n_get(MSG_ABOUT_PSP_SDK), (Rect){ctx, cty, ctw, 14}, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    cty += 14;
    ui_draw_text(SDK_VERSION, (Rect){ctx, cty, ctw, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);
    cty += 25;

    // Date
    ui_draw_text(i18n_get(MSG_ABOUT_DATE), (Rect){ctx, cty, ctw, 14}, COLOR_SUBTEXT, UI_FONT_SIZE_TINY, ALIGN_LEFT);
    cty += 14;
    struct tm b_tm = {0};
    b_tm.tm_year = BUILD_YEAR - 1900;
    b_tm.tm_mon = BUILD_MONTH - 1;
    b_tm.tm_mday = BUILD_DAY;
    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), i18n_get(MSG_DATE_FORMAT), &b_tm);
    ui_draw_text(date_buf, (Rect){ctx, cty, ctw, 14}, COLOR_TEXT, UI_FONT_SIZE_SMALL, ALIGN_LEFT);

    ui_draw_standard_hints();
}

Screen g_screen_about = {
    about_init,
    about_update,
    about_draw,
    NULL
};
