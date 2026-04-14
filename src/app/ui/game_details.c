#include "app/ui/screen.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include <pspctrl.h>
#include <stdio.h>

static int g_game_idx = 0;
static Texture* g_game_icon = NULL;

void game_details_set_idx(int idx) {
    g_game_idx = idx;
}

static void game_details_init(void) {
    if (g_game_icon) texture_free(g_game_icon);
    
    GameStats* games = data_get_games();
    char icon_path[256];
    snprintf(icon_path, sizeof(icon_path), "ms0:/PSP/COMMON/GameDiary/source/%s.png", games[g_game_idx].entry.game_id);
    g_game_icon = texture_load_png(icon_path);
}

static void game_details_update(u32 buttons, u32 pressed) {
    (void)buttons;
    if (pressed & PSP_CTRL_CIRCLE) screen_manager_set(&g_screen_game_list);
}

static void format_time(u32 seconds, char *out, size_t size) {
    u32 h = seconds / 3600;
    u32 m = (seconds % 3600) / 60;
    snprintf(out, size, "%luh %lum", h, m);
}

static void game_details_draw(void) {
    renderer_clear(0xFF101010);
    
    GameStats* games = data_get_games();
    GameStats* g = &games[g_game_idx];

    // Icon
    if (g_game_icon) {
        texture_draw(g_game_icon, 20, 20, 144, 80);
    } else {
        renderer_draw_rect(20, 20, 144, 80, 0xFF333333);
        font_draw_string(45, 65, "NO ICON", 0xFF888888, 0.8f);
    }

    font_draw_string(180, 45, g->entry.game_name, 0xFF00FFFF, 1.2f);
    font_draw_string(180, 75, g->entry.game_id, 0xFFAAAAAA, 0.9f);

    renderer_draw_rect(20, 120, 440, 120, 0xFF222222);
    
    char buf[64];
    font_draw_string(40, 150, i18n_get("stats.total_playtime"), 0xFFAAAAAA, 0.9f);
    format_time(g->total_playtime, buf, sizeof(buf));
    font_draw_string(250, 150, buf, 0xFFFFFFFF, 0.9f);

    font_draw_string(40, 180, i18n_get("stats.sessions"), 0xFFAAAAAA, 0.9f);
    snprintf(buf, sizeof(buf), "%lu", g->session_count);
    font_draw_string(250, 180, buf, 0xFFFFFFFF, 0.9f);

    font_draw_string(20, 260, i18n_get("ctrl.back"), 0xFF888888, 0.8f);
}

static void game_details_destroy(void) {
    if (g_game_icon) {
        texture_free(g_game_icon);
        g_game_icon = NULL;
    }
}

Screen g_screen_game_details = {
    game_details_init,
    game_details_update,
    game_details_draw,
    game_details_destroy
};
