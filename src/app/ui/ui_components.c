#include "app/ui/ui_components.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include <string.h>

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
            // Right align not implemented in font.h, fallback to left for now or just skip
            font_draw_string(r.x + r.w - 100, y_pos, text, color, size); 
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
