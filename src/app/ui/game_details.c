#include "app/ui/screen.h"
#include "app/ui/ui_components.h"
#include "app/ui/ui_layout.h"
#include "app/i18n.h"
#include "app/render/renderer.h"
#include "app/render/font.h"
#include "app/render/texture.h"
#include "app/data/data_loader.h"
#include "common/utils.h"
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

static void game_details_draw(void) {
    renderer_clear(COLOR_BG);

    GameStats* games = data_get_games();
    GameStats* g = &games[g_game_idx];

    // Card for the whole section
    Rect main_card = {20, 20, 440, 220};
    ui_draw_card(main_card, COLOR_CARD, COLOR_BORDER);

    // Icon (Centered in 144x80 box at x=40)
    sceGuColor(0xFFFFFFFF); // Important: reset color tint state
    if (g_game_icon) {
        int draw_w = g_game_icon->width;
        int draw_h = g_game_icon->height;
        int off_x = 40 + (144 - draw_w) / 2;
        int off_y = 40 + (80 - draw_h) / 2;
        texture_draw(g_game_icon, off_x, off_y, draw_w, draw_h);
    } else {
        const ImageResource* res = &GD_IMG_ICON_NOT_FOUND_PNG;
        int off_x = 40 + (144 - res->width) / 2;
        int off_y = 40 + (80 - res->height) / 2;
        texture_draw_resource(res, off_x, off_y, res->width, res->height);
    }

    // Title & ID
    ui_draw_text(g->entry.game_name, (Rect){200, 40, 240, 40}, COLOR_ACCENT, 1.2f, ALIGN_LEFT);
    ui_draw_text(g->entry.game_id, (Rect){200, 80, 240, 20}, COLOR_SUBTEXT, 0.8f, ALIGN_LEFT);

    // Separator
    renderer_draw_rect(40, 135, 400, 1, COLOR_BORDER);

    // Stats Grid inside card
    char buf[64];
    Rect stats_area = {40, 150, 400, 80};

    // Total Playtime row
    Rect row1 = rect_column(stats_area, 0, 2, 5);
    ui_draw_text(i18n_get(MSG_STATS_TOTAL_PLAYTIME), row1, COLOR_SUBTEXT, 0.8f, ALIGN_LEFT);
    utils_format_time(g->total_playtime, buf, sizeof(buf));
    ui_draw_text(buf, row1, COLOR_TEXT, 0.9f, ALIGN_RIGHT);

    // Sessions row
    Rect row2 = rect_column(stats_area, 1, 2, 5);
    ui_draw_text(i18n_get(MSG_STATS_SESSIONS), row2, COLOR_SUBTEXT, 0.8f, ALIGN_LEFT);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)g->session_count);
    ui_draw_text(buf, row2, COLOR_TEXT, 0.9f, ALIGN_RIGHT);
    const char* back_label = i18n_get(MSG_CTRL_BACK);
    char hint_o[64];

    snprintf(hint_o, sizeof(hint_o), "[%s] %s", UI_SYM_CIRCLE_OPEN, back_label);
    ui_draw_hint(hint_o, 10, 255, COLOR_SUBTEXT);
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
