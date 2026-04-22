/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef SDF_FONT_H
#define SDF_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optionally set the base directory where font atlases are located.
 * Must be called BEFORE sdf_font_init().
 * PPSSPP default: "assets/fonts" (relative, works out of the box)
 * Real PSP example: "ms0:/PSP/GAME/GameDiaryApp/assets/fonts"
 */
void sdf_font_set_assets_path(const char *path);

/**
 * Initialize the SDF font system. Loads atlases into VRAM and reads binary metadata.
 * It will load the Latin+Cyrillic, CJK, and Symbols fallback chain.
 * @return 0 on success, < 0 on failure.
 */
int sdf_font_init(void);

/**
 * Draw a UTF-8 string at the given position with the specified color and size.
 */
void sdf_font_draw_string(float x, float y, const char *str, uint32_t color, float size);

/**
 * Draw a UTF-8 string horizontally centered at the given position.
 */
void sdf_font_draw_string_centered(float x, float y, const char *str, uint32_t color, float size);

/**
 * Calculate total width of string in pixels.
 */
float sdf_font_get_width(const char *str, float size);

/**
 * Cleanup SDF font resources.
 */
void sdf_font_cleanup(void);

/**
 * Rebuild the glyph mapping table based on current language priority.
 * Called automatically by i18n_set_language().
 */
void sdf_font_rebuild_glyph_map(void);


#ifdef __cplusplus
}
#endif

#endif // SDF_FONT_H
