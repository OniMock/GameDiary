#include "app/ui/screen.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/data/data_loader.h"
#include "app/data/stats_calculator.h"
#include <pspctrl.h>
#include <stdio.h>

static int g_tab = 0; // 0: 7d, 1: 30d, 2: All

static void stats_init(void) {
    g_tab = 0;
    // Calculate 7 days ago timestamp
    // Assuming simple seconds for now
    u32 now = 0xFFFFFFFF; // Temporary hack until we have real time
    data_calculate_stats(now - 7 * 24 * 3600, now);
    stats_sort_by_period();
}

static void stats_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & PSP_CTRL_LTRIGGER) {
        g_tab = (g_tab + 2) % 3;
    }
    if (pressed & PSP_CTRL_RTRIGGER) {
        g_tab = (g_tab + 1) % 3;
    }
    
    if (pressed & PSP_CTRL_CIRCLE) screen_manager_set(&g_screen_dashboard);
}

static void stats_draw(void) {
    renderer_clear(0xFF101010);
    
    // Tabs
    const char* tabs[] = { i18n_get("top.week"), i18n_get("top.month"), i18n_get("top.all") };
    for (int i = 0; i < 3; i++) {
        uint32_t color = (i == g_tab) ? 0xFFFFFFFF : 0xFF555555;
        font_draw_string(50 + i * 150, 40, tabs[i], color, 1.0f);
    }
    
    renderer_draw_rect(20, 50, 440, 2, 0xFF333333);
    
    GameStats* games = data_get_games();
    u32 count = data_get_game_count();
    
    for (u32 i = 0; i < 5 && i < count; i++) {
        char rank[4];
        snprintf(rank, sizeof(rank), "%lu.", i + 1);
        font_draw_string(30, 80 + i * 35, rank, 0xFFAAAAAA, 0.9f);
        font_draw_string(60, 80 + i * 35, games[i].entry.game_name, 0xFFFFFFFF, 1.0f);
    }
}

Screen g_screen_stats = {
    stats_init,
    stats_update,
    stats_draw,
    NULL
};
