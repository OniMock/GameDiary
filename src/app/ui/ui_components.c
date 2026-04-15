#include <psptypes.h>
#include <psprtc.h>
#include "app/ui/ui_components.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/i18n.h"
#include "common/utils.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

static float g_graph_anim[7] = {0.0f};



void ui_draw_card(Rect r, u32 bg_color, u32 border_color) {
    // Draw thin border (1px)
    renderer_draw_rect(r.x - 1, r.y - 1, r.w + 2, r.h + 2, border_color);
    // Draw main body
    renderer_draw_rect(r.x, r.y, r.w, r.h, bg_color);
}

void ui_draw_text(const char* text, Rect r, u32 color, float size, UIAlign align) {
    if (!text) return;
    
    float y_pos = r.y + (r.h / 2.0f) + (size * 6.0f); // Simple vertical centering approximation
    
    switch (align) {
        case ALIGN_LEFT:
            font_draw_string(r.x, y_pos, text, color, size);
            break;
        case ALIGN_CENTER:
            font_draw_string_centered(r.x + (r.w / 2.0f), y_pos, text, color, size);
            break;
        case ALIGN_RIGHT:
            font_draw_string(r.x + r.w - font_get_width(text, size), y_pos, text, color, size); 
            break;
    }
}

void ui_draw_hint(const char* text, int x, int y, u32 color) {
    font_draw_string(x, y, text, color, 0.8f);
}

void ui_draw_title(const char* text, Rect r) {
    font_draw_string(r.x, r.y + 20, text, COLOR_ACCENT, 1.3f);
    // Underline
    renderer_draw_rect(r.x, r.y + 28, 60, 3, COLOR_ACCENT);
}

void ui_draw_weekly_graph(SessionEntry* sessions, int count) {
    u32 day_playtime[7] = {0};
    u32 now_ts = utils_get_timestamp();
    time_t now_val = (time_t)now_ts;
    
    // Normalize to start of today (local midnight)
    // We copy the struct because localtime returns a static pointer which might be overwritten
    struct tm today_tm = *localtime(&now_val);
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    time_t today_start = mktime(&today_tm);

    int found_any = 0;

    // Aggregate sessions for the last 7 days (including today)
    for (int i = 0; i < count; i++) {
        time_t s_time = (time_t)sessions[i].timestamp;
        
        // Check if session falls in the [Today-6, Today] window
        // s_time >= today_start - 6 days
        if (s_time >= (today_start - 6 * 86400) && s_time < (today_start + 86400)) {
            int days_ago;
            if (s_time >= today_start) {
                days_ago = 0;
            } else {
                days_ago = (today_start - s_time) / 86400 + 1;
            }

            if (days_ago >= 0 && days_ago < 7) {
                day_playtime[days_ago] += sessions[i].duration;
                if (sessions[i].duration > 0) found_any = 1;
            }
        }
    }

    // Graph Layout
    int gx = 60, gy = 170, gw = 360, gh = 80;
    int bar_w = 30;
    int spacing = (gw - (7 * bar_w)) / 6;

    // Draw Base Line
    renderer_draw_rect(gx - 10, gy, gw + 20, 1, COLOR_BORDER);

    if (!found_any) {
        Rect msg_rect = {gx, gy - 50, gw, 30};
        ui_draw_text(i18n_get(MSG_STATS_NO_ACTIVITY), msg_rect, COLOR_SUBTEXT, 0.8f, ALIGN_CENTER);
    }

    // Scaling and rendering
    u32 max_time = 3600; // Min scale 1h
    for (int i = 0; i < 7; i++) {
        if (day_playtime[i] > max_time) max_time = day_playtime[i];
    }

    for (int i = 0; i < 7; i++) {
        int idx = 6 - i; // idx 0 is today (rightmost), idx 6 is 6 days ago (leftmost)
        float target_h = ((float)day_playtime[idx] / max_time) * gh;
        
        // Simple animation state (could use a timer, but this frame-based approach is common in homebrew)
        g_graph_anim[idx] += (target_h - g_graph_anim[idx]) * 0.15f;
        
        int x = gx + i * (bar_w + spacing);
        int h = (int)g_graph_anim[idx];
        if (h < 2 && day_playtime[idx] > 0) h = 2;

        // Visual highlights: Today (idx=0) is bright, others are dimmed
        u32 bar_color = (idx == 0) ? COLOR_ACCENT : (COLOR_ACCENT & 0x44FFFFFF);
        u32 label_color = (idx == 0) ? COLOR_ACCENT : COLOR_SUBTEXT;

        if (h > 0) {
            renderer_draw_rect(x, gy - h, bar_w, h, bar_color);
            // "Glossy" top edge for today
            if (idx == 0 && h > 4) {
                renderer_draw_rect(x, gy - h, bar_w, 2, 0x88FFFFFF);
            } else if (h > 4) {
                renderer_draw_rect(x, gy - h, bar_w, 2, 0x22FFFFFF);
            }

            // Draw playtime text on top
            char time_buf[16];
            utils_format_duration_compact(day_playtime[idx], time_buf, sizeof(time_buf));
            Rect time_rect = {x - 5, gy - h - 14, bar_w + 10, 10};
            ui_draw_text(time_buf, time_rect, label_color, 0.6f, ALIGN_CENTER);
        }

        // Labels rotated based on today
        time_t bar_day_time = today_start - (idx * 86400);
        struct tm bar_tm = *localtime(&bar_day_time);
        const char* day_name = i18n_get(MSG_DAY_SUN + bar_tm.tm_wday);
        
        Rect label_rect = {x, gy + 8, bar_w, 20};
        ui_draw_text(day_name, label_rect, label_color, 0.7f, ALIGN_CENTER);
    }
}
