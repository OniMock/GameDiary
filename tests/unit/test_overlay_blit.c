/**
 * Unit tests for src/plugin/overlay_blit.c (BUG-03 Verification & Coverage)
 */

#include "../framework/test_framework.h"
#include "../framework/mock_pspsdk.h"
#include "plugin/overlay_blit.h"
#include <pspdisplay.h>
#include <string.h>

#define PSP_SCREEN_WIDTH  480
#define PSP_SCREEN_HEIGHT 272

#define TEST_MAKE_COLOR(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16) | 0xFF000000U)

static void test_overlay_colors_get_set(void) {
    overlay_blit_set_colors(0xFF112233U, 0xFF445566U);
    u32 fg = 0, bg = 0;
    overlay_blit_get_colors(&fg, &bg);
    ASSERT_EQ(fg, 0xFF112233U);
    ASSERT_EQ(bg, 0xFF445566U);
}

static void test_overlay_color_conversion_565_red_channel(void) {
    static u16 fb[512 * 272];
    memset(fb, 0, sizeof(fb));

    u32 pure_red_8888 = TEST_MAKE_COLOR(0xFF, 0x00, 0x00);
    overlay_blit_set_colors(0xFFFFFFFFU, pure_red_8888);

    overlay_blit_draw_to(fb, 512, PSP_DISPLAY_PIXEL_FORMAT_565, "A", NULL, 1);

    u16 pixel = fb[5 * 512 + 5];
    u16 red_val  = (pixel >> 11) & 0x1F;
    u16 blue_val = pixel & 0x1F;

    ASSERT_TRUE(red_val > 0);
    ASSERT_EQ(blue_val, 0U);
}

static void test_overlay_formats_and_multiline(void) {
    // 8888 format
    static u32 fb8888[512 * 272];
    memset(fb8888, 0, sizeof(fb8888));
    overlay_blit_set_colors(0xFFFFFFFFU, 0xFF223344U);
    overlay_blit_draw_to(fb8888, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, "Line 1 Test", "Line 2 Test", 2);
    ASSERT_TRUE(fb8888[5 * 512 + 5] != 0);

    // 5551 format
    static u16 fb5551[512 * 272];
    memset(fb5551, 0, sizeof(fb5551));
    overlay_blit_draw_to(fb5551, 512, PSP_DISPLAY_PIXEL_FORMAT_5551, "Line 1", "Line 2", 2);
    ASSERT_TRUE(fb5551[5 * 512 + 5] != 0);

    // 4444 format
    static u16 fb4444[512 * 272];
    memset(fb4444, 0, sizeof(fb4444));
    overlay_blit_draw_to(fb4444, 512, PSP_DISPLAY_PIXEL_FORMAT_4444, "Line 1", "Line 2", 2);
    ASSERT_TRUE(fb4444[5 * 512 + 5] != 0);

    // Default stride fallback when bufferwidth <= 0
    overlay_blit_draw_to(fb8888, 0, PSP_DISPLAY_PIXEL_FORMAT_8888, "Fallback Stride", NULL, 1);

    // Edge cases: NULL vram or invalid line_count
    overlay_blit_draw_to(NULL, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, "Test", NULL, 1);
    overlay_blit_draw_to(fb8888, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, "Test", NULL, 0);
}

static void test_overlay_wrapping_and_special_chars(void) {
    static u16 fb565[512 * 272];
    memset(fb565, 0, sizeof(fb565));

    // Long line > 60 chars triggers word wrap to next line
    const char *long_msg = "This is a remarkably long line of text designed to exceed sixty columns on screen!";
    overlay_blit_draw_to(fb565, 512, PSP_DISPLAY_PIXEL_FORMAT_565, long_msg, "Newline\nCarriage\rUnprintable\x01\x1F", 2);
    ASSERT_TRUE(fb565[5 * 512 + 5] != 0);

    // Unknown pixel format fallback branch
    overlay_blit_draw_to(fb565, 512, 999, "Unknown Pixel Format", NULL, 1);
}

static void test_overlay_blit_draw_all_and_fallbacks(void) {
    static u32 mock_vram[512 * 272];
    mock_display_set_active_fb(mock_vram, 512, PSP_DISPLAY_PIXEL_FORMAT_8888);

    overlay_blit_draw_all("HUD Line 1", "HUD Line 2", 2);
    overlay_blit_draw_all("Empty", NULL, 0); // line_count <= 0

    // Test fallbacks stub
    overlay_blit_draw_fallbacks("Test", NULL, 1);

    // When framebuffer is unavailable
    mock_display_set_active_fb(NULL, 0, 0);
    overlay_blit_draw_all("No Framebuffer", NULL, 1);
}

void run_test_overlay_blit(void) {
    TEST_SUITE_BEGIN("Overlay Blit & Color Conversions Module");
    TEST_RUN(test_overlay_colors_get_set);
    TEST_RUN(test_overlay_color_conversion_565_red_channel);
    TEST_RUN(test_overlay_formats_and_multiline);
    TEST_RUN(test_overlay_wrapping_and_special_chars);
    TEST_RUN(test_overlay_blit_draw_all_and_fallbacks);
    TEST_SUITE_END();
}
