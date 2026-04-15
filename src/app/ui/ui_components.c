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
    time_t now = (time_t)now_ts;
    
    // Normalize to start of today (local midnight)
    struct tm *lt = localtime(&now);
    int now_year = lt->tm_year;
    int now_yday = lt->tm_yday;
    
    // Reference for label generation
    lt->tm_hour = 0; lt->tm_min = 0; lt->tm_sec = 0;
    time_t today_start = mktime(lt);

    int found_any = 0;

    // Aggregate sessions
    for (int i = 0; i < count; i++) {
        time_t session_time = (time_t)sessions[i].timestamp;
        struct tm *st = localtime(&session_time);
        
        int days_ago = -1;
        if (st->tm_year == now_year) {
            days_ago = now_yday - st->tm_yday;
        } else if (st->tm_year == now_year - 1) {
            // Check for year wraparound
            days_ago = now_yday + ( (now_year % 4 == 0) ? 366 : 365 ) - st->tm_yday;
        }
        
        if (days_ago >= 0 && days_ago < 7) {
            day_playtime[days_ago] += sessions[i].duration;
            if (sessions[i].duration > 0) found_any = 1;
        }
    }

    // Graph Layout
    int gx = 60, gy = 170, gw = 360, gh = 80;
    int bar_w = 28;
    int spacing = (gw - (7 * bar_w)) / 6;

    // Draw Base Line
    renderer_draw_rect(gx - 10, gy, gw + 20, 1, COLOR_BORDER);

    if (!found_any) {
        Rect msg_rect = {gx, gy - 50, gw, 30};
        ui_draw_text(i18n_get(MSG_STATS_NO_ACTIVITY), msg_rect, COLOR_SUBTEXT, 0.8f, ALIGN_CENTER);
    }

    // Scaling and rendering
    u32 max_time = 3600; 
    for (int i = 0; i < 7; i++) {
        if (day_playtime[i] > max_time) max_time = day_playtime[i];
    }

    for (int i = 0; i < 7; i++) {
        int idx = 6 - i;
        float target_h = ((float)day_playtime[idx] / max_time) * gh;
        g_graph_anim[idx] += (target_h - g_graph_anim[idx]) * 0.1f;
        
        int x = gx + i * (bar_w + spacing);
        int h = (int)g_graph_anim[idx];
        if (h < 2 && day_playtime[idx] > 0) h = 2;

        u32 bar_color = (idx == 0) ? COLOR_ACCENT : (COLOR_ACCENT & 0x66FFFFFF);
        if (h > 0) {
            renderer_draw_rect(x, gy - h, bar_w, h, bar_color);
            if (h > 4) renderer_draw_rect(x, gy - h, bar_w, 2, 0x44FFFFFF);
        }

        time_t day_time = today_start - (idx * 86400);
        struct tm *dt = localtime(&day_time);
        const char* day_label = i18n_get(MSG_DAY_SUN + dt->tm_wday);
        Rect label_rect = {x, gy + 5, bar_w, 20};
        ui_draw_text(day_label, label_rect, COLOR_SUBTEXT, 0.7f, ALIGN_CENTER);
    }
}
