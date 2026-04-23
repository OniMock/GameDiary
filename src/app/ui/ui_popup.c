/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/ui/ui_popup.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/ui/ui_layout.h"
#include "app/ui/ui_components.h"
#include <pspctrl.h>
#include <pspgu.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define MAX_VISIBLE_LINES 128
#define MAX_LINE_WIDTH 512

typedef enum {
    POPUP_STATE_CLOSED = 0,
    POPUP_STATE_OPENING,
    POPUP_STATE_OPEN,
    POPUP_STATE_CLOSING
} PopupState;

static char s_wrapped_lines[MAX_VISIBLE_LINES][MAX_LINE_WIDTH];
static int s_wrapped_count = 0;

static PopupState s_state = POPUP_STATE_CLOSED;
static float s_alpha = 0.0f;
static const PopupData* s_current_data = NULL;
static int s_scroll_offset = 0;
static float s_line_height = 0.0f;

/* CJK byte identification */
static size_t utf8_char_size(unsigned char c) {
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void wrap_and_append_line(const char* src, float scale, int max_px_width) {
    if (!src) return;

    size_t i = 0;
    while (src[i] != '\0' && s_wrapped_count < MAX_VISIBLE_LINES) {
        float current_w = 0.0f;
        size_t last_space_i = 0;
        size_t current_segment_start = i;
        int found_space = 0;
        char temp_char[5];

        size_t cursor = i;
        size_t last_fitting_cursor = cursor;

        while (src[cursor] != '\0') {
            size_t sz = utf8_char_size((unsigned char)src[cursor]);
            memcpy(temp_char, &src[cursor], sz);
            temp_char[sz] = '\0';

            float char_w = font_get_width(temp_char, scale);
            if (current_w + char_w > max_px_width && cursor > current_segment_start) {
                // Width exceeded, must break here or at the last space
                break;
            }

            current_w += char_w;
            if (src[cursor] == ' ') {
                found_space = 1;
                last_space_i = cursor;
            }

            last_fitting_cursor = cursor + sz;
            cursor += sz;
        }

        if (src[cursor] == '\0') {
            // Fits entirely in remaining space
            size_t len = cursor - current_segment_start;
            if (len >= MAX_LINE_WIDTH) len = MAX_LINE_WIDTH - 1;
            memcpy(s_wrapped_lines[s_wrapped_count], &src[current_segment_start], len);
            s_wrapped_lines[s_wrapped_count][len] = '\0';
            s_wrapped_count++;
            break;
        } else {
            // Needs breaking
            size_t break_pt = 0;
            if (found_space) {
                // Break at the last seen space
                break_pt = last_space_i;
            } else {
                // Break at exact char (CJK or very long word)
                break_pt = last_fitting_cursor;
            }

            size_t len = break_pt - current_segment_start;
            if (len >= MAX_LINE_WIDTH) len = MAX_LINE_WIDTH - 1;
            memcpy(s_wrapped_lines[s_wrapped_count], &src[current_segment_start], len);
            s_wrapped_lines[s_wrapped_count][len] = '\0';
            s_wrapped_count++;

            i = break_pt;
            // Skip the space we broke on, don't carry it to next line
            if (found_space && src[i] == ' ') i++;
        }
    }
}

void popup_open(const PopupData* data) {
    if (!data) return;

    s_current_data = data;
    s_wrapped_count = 0;
    s_scroll_offset = 0;

    // Cache font metric
    s_line_height = font_get_height(0.8f) + 4.0f; // Size logic plus 4px gap

    // Box constraints
    int text_box_w = 420; // 450 card - 30 padding

    for (int i = 0; i < data->line_count; i++) {
        const char* l = data->lines[i];
        if (!l) continue;
        wrap_and_append_line(l, 0.8f, text_box_w);
    }

    s_state = POPUP_STATE_OPENING;
    s_alpha = 0.0f;
}

void popup_close(void) {
    if (s_state == POPUP_STATE_OPEN || s_state == POPUP_STATE_OPENING) {
        s_state = POPUP_STATE_CLOSING;
    }
}

int popup_is_open(void) {
    return s_state != POPUP_STATE_CLOSED;
}

void popup_update(uint32_t buttons, uint32_t pressed) {
    // Fade in/out logic
    if (s_state == POPUP_STATE_OPENING) {
        s_alpha += 0.15f;
        if (s_alpha >= 1.0f) {
            s_alpha = 1.0f;
            s_state = POPUP_STATE_OPEN;
        }
    } else if (s_state == POPUP_STATE_CLOSING) {
        s_alpha -= 0.15f;
        if (s_alpha <= 0.0f) {
            s_alpha = 0.0f;
            s_state = POPUP_STATE_CLOSED;
            s_current_data = NULL; // Release pointer hook safely
        }
    }

    if (s_state == POPUP_STATE_OPEN) {
        // Exit conditions
        if (pressed & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE)) {
            popup_close();
            return;
        }

        // Scroll Logic
        int text_area_h = 160;
        int max_visible = text_area_h / (int)s_line_height;
        int max_scroll = s_wrapped_count - max_visible;
        if (max_scroll < 0) max_scroll = 0;

        static int scroll_timer = 0;
        int move = 0;

        if (buttons & PSP_CTRL_UP) move = -1;
        else if (buttons & PSP_CTRL_DOWN) move = 1;

        if (move != 0) {
            if (scroll_timer == 0 || scroll_timer == -15) {
                s_scroll_offset += move;
                if (scroll_timer == 0) scroll_timer = -15; // Delay before repeat
                else scroll_timer = 3; // Fast repeat length
            } else {
                if (scroll_timer < 0) scroll_timer++;
                else scroll_timer--;
            }
        } else {
            scroll_timer = 0;
        }

        // Clamp scrolling
        if (s_scroll_offset > max_scroll) s_scroll_offset = max_scroll;
        if (s_scroll_offset < 0) s_scroll_offset = 0;
    }
}

void popup_render(void) {
    if (s_state == POPUP_STATE_CLOSED) return;

    // --- Backdrop Overlay ---
    // RGB 0x000000, max alpha 51 (20% of 255)
    uint32_t bg_alpha = (uint32_t)(51.0f * s_alpha);
    if (bg_alpha > 255) bg_alpha = 255;
    uint32_t background_tint = (bg_alpha << 24) | 0x000000;
    renderer_draw_rect(0, 0, 480, 272, background_tint);

    // --- Box Geometry ---
    // Centralized popup dimensions
    int popup_w = 450;
    int popup_h = 242;
    int x_origin = 15;
    int y_origin = 15;

    uint32_t element_alpha = (uint32_t)(255.0f * s_alpha);
    uint32_t card_bg     = (element_alpha << 24) | 0x111111;
    uint32_t card_border = (element_alpha << 24) | 0x555555;
    uint32_t text_color  = (element_alpha << 24) | 0xFFFFFF;
    uint32_t line_color  = (element_alpha << 24) | 0x333333;

    Rect box_rect = { x_origin, y_origin, popup_w, popup_h };
    ui_draw_card(box_rect, card_bg, card_border);

    // --- Header ---
    int header_y = y_origin + 15;
    int text_offset_x = x_origin + 20;

    if (s_current_data && s_current_data->icon) {
        int icon_size = 24;
        // Re-using tint logic, Gu color must include alpha for standard textures.
        sceGuColor(text_color);
        texture_draw_resource(s_current_data->icon, text_offset_x, header_y, icon_size, icon_size);
        text_offset_x += icon_size + 10;
    }

    if (s_current_data && s_current_data->title) {
        // Center text vertically relative to the 24px icon.
        Rect title_rect = { text_offset_x, header_y + 4, popup_w - 60, 24 };
        ui_draw_text(s_current_data->title, title_rect, text_color, 1.0f, ALIGN_LEFT);
    }

    // Header Separator
    int sep_y = y_origin + 50;
    renderer_draw_rect(x_origin + 15, sep_y, popup_w - 30, 1, line_color);

    // --- Text Area (Scrollable body) ---
    int text_area_y = sep_y + 10;
    int text_area_h = popup_h - 50 - 10 - 20; // 50(hdr) + 10(ptop) + 20(pbot) = 162

    int max_visible = text_area_h / (int)s_line_height;

    uint32_t body_alpha = (uint32_t)(170.0f * s_alpha); // 0xAA is 170
    uint32_t body_color = (body_alpha << 24) | (COLOR_SUBTEXT & 0x00FFFFFF);

    // Safety crop loop logic
    for (int i = 0; i < max_visible && (s_scroll_offset + i) < s_wrapped_count; i++) {
        int line_idx = s_scroll_offset + i;
        int render_y = text_area_y + (i * (int)s_line_height);

        Rect t_rect = { x_origin + 15, render_y, popup_w - 30, (int)s_line_height };
        ui_draw_text(s_wrapped_lines[line_idx], t_rect, body_color, 0.8f, ALIGN_LEFT);
    }

    // --- Scroll Indicators ---
    if (s_scroll_offset > 0) {
        Rect up_rect = { 0, text_area_y - 12, 480, 12 };
        ui_draw_text("~", up_rect, text_color, 0.7f, ALIGN_CENTER); // Basic Triangle pointer / indicator
    }

    int max_scroll = s_wrapped_count - max_visible;
    if (max_scroll > 0 && s_scroll_offset < max_scroll) {
        Rect down_rect = { 0, text_area_y + text_area_h + 2, 480, 12 };
        ui_draw_text("v", down_rect, text_color, 0.7f, ALIGN_CENTER); // Basic downward indicator
    }
}
