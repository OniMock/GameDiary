#include "app/ui/screen.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/data/data_loader.h"
#include <pspctrl.h>
#include <stdio.h>

static void dashboard_init(void) {
    // Recalculate stats for "All Time"
    data_calculate_stats(0, 0xFFFFFFFF);
}

static void dashboard_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & PSP_CTRL_RTRIGGER) screen_manager_set(&g_screen_stats);
    if (pressed & PSP_CTRL_LTRIGGER) screen_manager_set(&g_screen_settings);
    if (pressed & PSP_CTRL_CROSS)    screen_manager_set(&g_screen_game_list);
}

static void format_time(u32 seconds, char *out, size_t size) {
    u32 h = seconds / 3600;
    u32 m = (seconds % 3600) / 60;
    snprintf(out, size, "%luh %lum", h, m);
}

static void dashboard_draw(void) {
    renderer_clear(0xFF1A1A1A);
    
    // Header
    font_draw_string(20, 30, i18n_get("app.title"), 0xFF00AAFF, 1.3f);
    
    // Summary Panel
    renderer_draw_rect(20, 60, 440, 100, 0xFF333333);
    font_draw_string(40, 85, i18n_get("stats.total_playtime"), 0xFFAAAAAA, 0.9f);
    
    u32 total_play = 0;
    GameStats* games = data_get_games();
    u32 count = data_get_game_count();
    for(u32 i=0; i<count; i++) total_play += games[i].total_playtime;
    
    char time_str[32];
    format_time(total_play, time_str, sizeof(time_str));
    font_draw_string(40, 120, time_str, 0xFFFFFFFF, 1.8f);
    
    // Navigation
    font_draw_string(20, 240, "[L] Settings", 0xFF888888, 0.8f);
    font_draw_string(380, 240, "Stats [R]", 0xFF888888, 0.8f);
    font_draw_string_centered(240, 200, i18n_get("menu.games_press_x"), 0xFFFFFFFF, 1.0f);
}

Screen g_screen_dashboard = {
    dashboard_init,
    dashboard_update,
    dashboard_draw,
    NULL
};
