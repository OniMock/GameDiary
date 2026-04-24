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
#include "app/ui/ui_text.h"
#include "app/ui/ui_popup.h"
#include "app/i18n/i18n.h"
#include "app/render/renderer.h"
#include "app/render/texture.h"
#include "app/render/font.h"
#include "app/render/image_resources.h"
#include <pspctrl.h>
#include <pspgu.h>
#include <stdio.h>

static const char* s_helper_lines[5];
static PopupData s_helper_data;

static void support_init(void) {
    s_helper_lines[0] = i18n_get(MSG_HELP_CONTROLS);
    s_helper_lines[1] = i18n_get(MSG_HELP_BTN_O_BACK);
    s_helper_lines[2] = "";
    s_helper_lines[3] = i18n_get(MSG_HELP_INFO_LABEL);
    s_helper_lines[4] = i18n_get(MSG_HELP_DESC_SUPPORT);

    s_helper_data.title = i18n_get(MSG_HELP_TITLE);
    s_helper_data.icon = &GD_IMG_ICON_HELPER_32_PNG;
    s_helper_data.lines = s_helper_lines;
    s_helper_data.line_count = 5;
    s_helper_data.show_close_hint = true;
}

static void support_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE)) {
        screen_manager_pop();
    }
    if (pressed & PSP_CTRL_LTRIGGER) {
        popup_open(&s_helper_data);
    }
}

static void support_draw(void) {
    renderer_clear(COLOR_BG);

    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);

    // Header
    ui_draw_title_auto(i18n_get(MSG_SETTINGS_SUPPORT), safe_rect, &GD_IMG_ICON_SUPPORT_32_PNG);

    int content_x = safe_rect.x + 10;
    int content_y = safe_rect.y + 28; // Lifted up to the limit below title line
    int content_w = safe_rect.w - 20;

    // --- Card 1: Description (Top) ---
    // Reduced height from 65 to 55 to give more space
    Rect card1_rect = { content_x, content_y, content_w, 55 };
    ui_draw_card(card1_rect, COLOR_CARD, COLOR_BORDER);

    int tx = card1_rect.x + 15;
    int ty = card1_rect.y + 6; // Pulled text up slightly
    int tw = card1_rect.w - 30;

    char wrapped[6][MAX_LINE_WIDTH];
    int line_count = 0;
    ui_text_wrap(i18n_get(MSG_SUPPORT_DESC), UI_FONT_SIZE_NORMAL, tw, wrapped, 6, &line_count);

    for (int i = 0; i < line_count; i++) {
        ui_draw_text(wrapped[i], (Rect){tx, ty, tw, 14}, COLOR_TEXT, UI_FONT_SIZE_NORMAL, ALIGN_LEFT);
        ty += 16;
    }

    // --- Bottom Row: Two Cards Side-by-side ---
    // Reduced gap between cards to 8
    int card_btm_y = card1_rect.y + card1_rect.h + 8;
    int card_w = (content_w - 10) / 2;

    // QR Coffee
    // Reduced height from 140 to 124
    Rect card_coffee = { content_x, card_btm_y, card_w, 124 };
    ui_draw_card(card_coffee, COLOR_CARD, COLOR_BORDER);

    ui_draw_text(i18n_get(MSG_SUPPORT_COFFEE), (Rect){card_coffee.x, card_coffee.y + 6, card_coffee.w, 14}, COLOR_ACCENT, UI_FONT_SIZE_SMALL, ALIGN_CENTER);

    // Reset color to white so QR codes are not tinted (prevents them from staying blue/accent)
    sceGuColor(0xFFFFFFFF);

    // Scaled QR down to 96 to fit well within shorter card
    int qr_size = 96;
    int qr_y_off = 22;
    int qr_x_off = (card_coffee.w - qr_size) / 2;
    texture_draw_resource(&GD_IMG_QR_COFFEE_128_PNG, card_coffee.x + qr_x_off, card_coffee.y + qr_y_off, qr_size, qr_size);

    // QR Wallet
    Rect card_wallet = { content_x + card_w + 10, card_btm_y, card_w, 124 };
    ui_draw_card(card_wallet, COLOR_CARD, COLOR_BORDER);

    ui_draw_text(i18n_get(MSG_SUPPORT_WALLET), (Rect){card_wallet.x, card_wallet.y + 6, card_wallet.w, 14}, COLOR_ACCENT, UI_FONT_SIZE_SMALL, ALIGN_CENTER);

    // Ensure white for second QR too
    sceGuColor(0xFFFFFFFF);

    qr_x_off = (card_wallet.w - qr_size) / 2;
    texture_draw_resource(&GD_IMG_QR_WALLET_128_PNG, card_wallet.x + qr_x_off, card_wallet.y + qr_y_off, qr_size, qr_size);

    ui_draw_standard_hints();
}

Screen g_screen_support = {
    support_init,
    support_update,
    support_draw,
    NULL
};
