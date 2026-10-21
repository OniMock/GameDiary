/**
 * Mock stubs for decoupling tests from PSP-specific UI and rendering engines.
 */

#include "app/render/image_resources.h"
#include "app/render/texture.h"
#include "app/config/config.h"
#include "app/ui/carousel_state.h"
#include "app/i18n/i18n.h"
#include <stdio.h>
#include <stdlib.h>

/* ImageResource instances for i18n flags */
const ImageResource GD_IMG_FLAG_CN_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_DE_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_EN_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_ES_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_FR_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_IT_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_JP_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_PT_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};
const ImageResource GD_IMG_FLAG_RU_PNG = {0, 0, 0, 0, 0, 0, 0, NULL};

/* Font stub */
void sdf_font_rebuild_glyph_map(void) {
    // No-op in test harness
}

/* Worker Thread stubs */
int worker_enqueue_save_config(const AppConfig* cfg) {
    (void)cfg;
    return -1; // Fallback to synchronous write
}

int worker_enqueue_load_icon(CarouselState *cs, int slot_idx, int inf_idx, const char* path) {
    (void)cs; (void)slot_idx; (void)inf_idx; (void)path;
    return 0;
}

/* Texture stubs */
Texture *texture_load_png(const char *path) {
    (void)path;
    return NULL;
}

void texture_free(Texture *tex) {
    if (tex) free(tex);
}

/* Overlay notification stub for metadata_repository UMD check */
int overlay_notification_get_fb_count(void) {
    return 2; // Active double buffering
}

/* UI Duration Formatter (as defined in src/app/ui/ui_components.c) */
void ui_format_duration(u32 seconds, char *out, size_t size) {
    if (!out || size == 0) return;

    AppConfig *cfg = config_get();
    int hours_only = cfg ? cfg->format_hours_only : 0;

    if (hours_only) {
        u32 h = seconds / 3600;
        u32 m = (seconds % 3600) / 60;
        if (h > 0) {
            if (m > 0) {
                snprintf(out, size, "%uh %um", h, m);
            } else {
                snprintf(out, size, "%uh", h);
            }
        } else {
            snprintf(out, size, "%um", m);
        }
        return;
    }

    u32 d = seconds / 86400;
    u32 h = (seconds % 86400) / 3600;
    u32 m = (seconds % 3600) / 60;

    if (d > 0) {
        snprintf(out, size, "%ud %uh %um", d, h, m);
    } else if (h > 0) {
        if (m > 0) {
            snprintf(out, size, "%uh %um", h, m);
        } else {
            snprintf(out, size, "%uh", h);
        }
    } else {
        snprintf(out, size, "%um", m);
    }
}
