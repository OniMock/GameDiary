/**
 * Unit tests for src/plugin/overlay_blit.c (BUG-03 Verification)
 */

#include "../framework/test_framework.h"
#include "plugin/overlay_blit.h"
#include <pspdisplay.h>
#include <string.h>

#define PSP_SCREEN_WIDTH  480
#define PSP_SCREEN_HEIGHT 272

/* PSP 8888 definition from overlay_notification.c:
 * #define OVERLAY_MAKE_COLOR(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | 0xFF000000U) */
#define TEST_MAKE_COLOR(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | 0xFF000000U)

static void test_overlay_colors_get_set(void) {
    overlay_blit_set_colors(0xFF112233U, 0xFF445566U);
    u32 fg = 0, bg = 0;
    overlay_blit_get_colors(&fg, &bg);
    ASSERT_EQ(fg, 0xFF112233U);
    ASSERT_EQ(bg, 0xFF445566U);
}

static void test_overlay_color_conversion_565_red_channel(void) {
    /* 512x272 16-bit framebuffer */
    static u16 fb[512 * 272];
    memset(fb, 0, sizeof(fb));

    /* Configure background to Pure Red in PSP 8888 (R in byte 0, B in byte 2) */
    u32 pure_red_8888 = TEST_MAKE_COLOR(0xFF, 0x00, 0x00);
    overlay_blit_set_colors(0xFFFFFFFFU, pure_red_8888);

    /* Render text to 565 framebuffer */
    overlay_blit_draw_to(fb, 512, PSP_DISPLAY_PIXEL_FORMAT_565, "A", NULL, 1);

    /* Sample a background pixel rendered inside the HUD box (x=5, y=5) */
    u16 pixel = fb[5 * 512 + 5];

    /* In standard 16-bit RGB 565 format:
     * Bits 11..15: Red (5 bits)
     * Bits  5..10: Green (6 bits)
     * Bits  0..4 : Blue (5 bits) */
    u16 red_val  = (pixel >> 11) & 0x1F;
    u16 blue_val = pixel & 0x1F;

    /* BUG-03 Detection:
     * In convert_psp8888_to_565(), Red is read from (color >> 16) & 0xFF (Blue byte)
     * and Blue is read from (color & 0xFF) (Red byte).
     * Therefore, for Pure Red, red_val evaluates to 0 and blue_val evaluates to 0x1F!
     * This assertion verifies that the red channel is non-zero, catching the inversion bug. */
    ASSERT_TRUE(red_val > 0);
    ASSERT_EQ(blue_val, 0U);
}

void run_test_overlay_blit(void) {
    TEST_SUITE_BEGIN("Overlay Blit & Color Conversions Module");
    TEST_RUN(test_overlay_colors_get_set);
    TEST_RUN(test_overlay_color_conversion_565_red_channel);
    TEST_SUITE_END();
}
