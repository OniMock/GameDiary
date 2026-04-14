#include "app/ui/screen.h"
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
    if (g_selection >= g_scroll + 6) g_scroll = g_selection - 5;

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
    renderer_clear(0xFF151515);
    font_draw_string(20, 30, i18n_get("menu.games"), 0xFFFFFFFF, 1.2f);

    GameStats* games = data_get_games();
    u32 count = data_get_game_count();

    if (count == 0) {
        font_draw_string_centered(240, 136, "No games found", 0xFF888888, 1.0f);
        return;
    }

    for (int i = 0; i < 6; i++) {
        int idx = g_scroll + i;
        if (idx >= (int)count) break;

        int y = 60 + i * 32;
        uint32_t color = (idx == g_selection) ? 0xFF00FFFF : 0xFFCCCCCC;
        
        if (idx == g_selection) {
            renderer_draw_rect(15, y - 20, 450, 30, 0xFF444444);
        }

        font_draw_string(30, y, games[idx].entry.game_name, color, 0.9f);
        
        char time_str[16];
        format_time(games[idx].total_playtime, time_str, sizeof(time_str));
        font_draw_string(380, y, time_str, color, 0.8f);
    }
}

Screen g_screen_game_list = {
    game_list_init,
    game_list_update,
    game_list_draw,
    NULL
};
