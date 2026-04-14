#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/data/data_loader.h"
#include <pspctrl.h>
#include <stdio.h>

static int g_scroll = 0;
static int g_selection = 0;

static void game_list_init(void) {
    g_selection = 0;
    g_scroll = 0;
}

static void game_list_update(u32 buttons, u32 pressed) {
    (void)buttons;
    u32 count = data_get_game_count();
    if (count == 0) {
        if (pressed & PSP_CTRL_CIRCLE) screen_manager_set(&g_screen_dashboard);
        return;
    }

    if (pressed & PSP_CTRL_UP) g_selection--;
    if (pressed & PSP_CTRL_DOWN) g_selection++;

    if (g_selection < 0) g_selection = count - 1;
    if (g_selection >= (int)count) g_selection = 0;

    // Scrolling logic
    if (g_selection < g_scroll) g_scroll = g_selection;
    if (g_selection >= g_scroll + 5) g_scroll = g_selection - 4;

    if (pressed & PSP_CTRL_CIRCLE) screen_manager_set(&g_screen_dashboard);
    
    if (pressed & PSP_CTRL_CROSS) {
        game_details_set_idx(g_selection);
        screen_manager_set(&g_screen_game_details);
    }
}

static void format_time(u32 seconds, char *out, size_t size) {
    u32 h = seconds / 3600;
    u32 m = (seconds % 3600) / 60;
    snprintf(out, size, "%luh %lum", (unsigned long)h, (unsigned long)m);
}

static void game_list_draw(void) {
    renderer_clear(COLOR_BG);
    
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);
    
    ui_draw_title(i18n_get("menu.games"), safe_rect);

    GameStats* games = data_get_games();
    u32 count = data_get_game_count();

    if (count == 0) {
        ui_draw_text("No games found", (Rect){0, 136, 480, 20}, COLOR_SUBTEXT, 1.0f, ALIGN_CENTER);
        return;
    }

    Rect list_area = {30, 80, 420, 150};
    for (int i = 0; i < 5; i++) {
        int idx = g_scroll + i;
        if (idx >= (int)count) break;

        Rect item_rect = rect_column(list_area, i, 5, 5);
        if (idx == g_selection) {
            ui_draw_card(item_rect, COLOR_HIGHLIGHT, COLOR_ACCENT);
        } else {
            ui_draw_card(item_rect, COLOR_CARD, COLOR_BORDER);
        }

        u32 color = (idx == g_selection) ? COLOR_ACCENT : COLOR_TEXT;
        Rect content = rect_padding(item_rect, 5);
        
        ui_draw_text(games[idx].entry.game_name, content, color, 0.8f, ALIGN_LEFT);
        
        char time_str[16];
        format_time(games[idx].total_playtime, time_str, sizeof(time_str));
        ui_draw_text(time_str, content, color, 0.7f, ALIGN_RIGHT);
    }

    ui_draw_hint(i18n_get("ctrl.back"), 20, 255, COLOR_SUBTEXT);
    ui_draw_hint(i18n_get("ctrl.select"), 390, 255, COLOR_SUBTEXT);
}

Screen g_screen_game_list = {
    game_list_init,
    game_list_update,
    game_list_draw,
    NULL
};
