/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/data/game_category.h"
#include "app/data/data_loader.h"
#include "app/i18n/i18n.h"
#include <string.h>

u8 game_category_normalize(u8 category) {
    if (category == CAT_VSH) return CAT_PSP;
    if (category > CAT_UNKNOWN) return CAT_UNKNOWN;
    return category;
}

const char* game_category_get_name(u8 category) {
    u8 norm = game_category_normalize(category);

    switch (norm) {
        case CAT_PSP:      return i18n_get(MSG_CAT_PSP);
        case CAT_PS1:      return i18n_get(MSG_CAT_PSX);
        case CAT_HOMEBREW: return i18n_get(MSG_CAT_HOMEBREW);
        default:           return i18n_get(MSG_CAT_HOMEBREW); // Fallback to HB for unknown
    }
}

int game_category_get_available(u8 *out_categories) {
    u32 count = data_get_game_count();
    GameStats *games = data_get_games();
    
    u8 found[CAT_UNKNOWN] = {0};
    int found_count = 0;

    for (u32 i = 0; i < count; i++) {
        u8 norm = game_category_normalize(games[i].entry.category);
        if (norm < CAT_UNKNOWN && found[norm] == 0) {
            found[norm] = 1;
            out_categories[found_count++] = norm;
        }
    }

    return found_count;
}
