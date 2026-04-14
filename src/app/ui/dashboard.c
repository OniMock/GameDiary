#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/data/data_loader.h"
#include <pspctrl.h>
#include <stdio.h>

static void dashboard_init(void) {
    data_calculate_stats(0, 0xFFFFFFFF);
}

static void dashboard_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & PSP_CTRL_RTRIGGER) screen_manager_set(&g_screen_settings);
    if (pressed & PSP_CTRL_LTRIGGER) screen_manager_set(&g_screen_stats);
    if (pressed & PSP_CTRL_CROSS)    screen_manager_set(&g_screen_game_list);
}

static void format_time(u32 seconds, char *out, size_t size) {
    u32 h = seconds / 3600;
    u32 m = (seconds % 3600) / 60;
    snprintf(out, size, "%luh %lum", (unsigned long)h, (unsigned long)m);
}

static void dashboard_draw(void) {
    renderer_clear(COLOR_BG);
    
    Rect screen_rect = {0, 0, 480, 272};
    Rect safe_rect = rect_padding(screen_rect, 20);
    
    // Header
    ui_draw_title(i18n_get(MSG_APP_TITLE), safe_rect);
    
    // Summary Card (Center)
    Rect card_rect = {40, 70, 400, 110};
    ui_draw_card(card_rect, COLOR_CARD, COLOR_BORDER);
    
    Rect card_content = rect_padding(card_rect, 15);
    Rect label_rect = rect_column(card_content, 0, 2, 0);
    Rect value_rect = rect_column(card_content, 1, 2, 0);
    
    ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), label_rect, COLOR_SUBTEXT, 0.8f, ALIGN_LEFT);
    
    u32 total_play = 0;
    GameStats* games = data_get_games();
    u32 count = data_get_game_count();
    for(u32 i=0; i<count; i++) total_play += games[i].total_playtime;
    
    char time_str[32];
    format_time(total_play, time_str, sizeof(time_str));
    ui_draw_text(time_str, value_rect, COLOR_TEXT, 1.6f, ALIGN_LEFT);
    
    // Interaction Hint
    ui_draw_text(i18n_get(MSG_MENU_GAMES_PRESS_X), (Rect){0, 190, 480, 30}, COLOR_TEXT, 0.9f, ALIGN_CENTER);
    
    // Footer Hints
    ui_draw_hint("[L] Settings", 20, 255, COLOR_SUBTEXT);
    ui_draw_hint("Stats [R]", 390, 255, COLOR_SUBTEXT);
}

Screen g_screen_dashboard = {
    dashboard_init,
    dashboard_update,
    dashboard_draw,
    NULL
};
