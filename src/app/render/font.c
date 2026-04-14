#include "app/render/font.h"
#include <intraFont.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <stdio.h>

static intraFont *g_font_main = NULL;

int font_init(void) {
    intraFontInit();
    
    // Load Latin-8 font which has better support for extended characters (accents)
    g_font_main = intraFontLoad("flash0:/font/ltn8.pgf", INTRAFONT_CACHE_ALL);
    if (!g_font_main) {
        // Fallback to default if ltn8 fails
        g_font_main = intraFontLoad("flash0:/font/ltn0.pgf", INTRAFONT_CACHE_ALL);
    }
    
    if (!g_font_main) return -1;
    
    return 0;
}

void font_draw_string(float x, float y, const char *str, uint32_t color, float size) {
    if (!g_font_main) return;
    
    // Set linear filtering for smoother text (prevents "blown out" look)
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    
    intraFontSetStyle(g_font_main, size, color, 0, 0.0f, INTRAFONT_ALIGN_LEFT);
    intraFontPrint(g_font_main, x, y, str);
}

void font_draw_string_centered(float x, float y, const char *str, uint32_t color, float size) {
    if (!g_font_main) return;
    
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    
    intraFontSetStyle(g_font_main, size, color, 0, 0.0f, INTRAFONT_ALIGN_CENTER);
    intraFontPrint(g_font_main, x, y, str);
}

void font_cleanup(void) {
    if (g_font_main) {
        intraFontUnload(g_font_main);
        g_font_main = NULL;
    }
    intraFontShutdown();
}
