#ifndef GAMEDIARY_TEXTURE_H
#define GAMEDIARY_TEXTURE_H

/**
 * @file texture.h
 * @brief Texture loading and rendering utilities.
 */
#include "app/render/image_resources.h"

typedef struct {
    int width;
    int height;
    int textureWidth;
    int textureHeight;
    void* data;
} Texture;

/**
 * @brief Loads a PNG file into a texture.
 * @param filename Path to the PNG file.
 * @return Pointer to Texture, or NULL on failure.
 */
Texture* texture_load_png(const char* filename);

/**
 * @brief Draws a texture on the screen.
 * @param tex Pointer to the texture.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width to draw.
 * @param h Height to draw.
 */
void texture_draw(Texture* tex, int x, int y, int w, int h);

/**
 * @brief Draws an embedded ImageResource on the screen.
 * @param res Pointer to the ImageResource.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param w Width to draw.
 * @param h Height to draw.
 */
void texture_draw_resource(const ImageResource* res, int x, int y, int w, int h);

/**
 * @brief Frees texture resources.
 * @param tex Pointer to the texture.
 */
void texture_free(Texture* tex);

/**
 * @brief Draws a texture with a custom tint/alpha color.
 * @param tex   Pointer to the texture.
 * @param x,y   Top-left screen position.
 * @param w,h   Draw dimensions.
 * @param color ARGB tint color (0xAARRGGBB). 0xFFFFFFFF = no tint.
 */
void texture_draw_tinted(Texture* tex, int x, int y, int w, int h, u32 color);

#endif // GAMEDIARY_TEXTURE_H
