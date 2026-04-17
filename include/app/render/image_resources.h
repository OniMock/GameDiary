/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  Copyright (c) 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
*/

#ifndef GAMEDIARY_IMAGE_RESOURCES_H
#define GAMEDIARY_IMAGE_RESOURCES_H

#include <stdint.h>

// PSP GU format constants if not using pspgu.h
//#ifndef GU_PSM_8888
//#define GU_PSM_8888 3
//#endif

#include <pspgu.h>

typedef struct {
    uint16_t width;      // Original width
    uint16_t height;     // Original height
    uint16_t pot_width;  // Next power of two width (for GPU)
    uint16_t pot_height; // Next power of two height (for GPU)
    uint16_t stride;     // Texture buffer width (multiple of 16)
    uint16_t format;
    uint32_t size;       // Total size in bytes (pot_width * pot_height * 4)
    const uint32_t* data;
} ImageResource;

#define GD_IMAGE_SIZE(img) ((img)->size)
#define GD_IMAGE_BYTES(img) ((img)->size)

extern const ImageResource GD_IMG_FLAG_EN_PNG;
extern const ImageResource GD_IMG_FLAG_ES_PNG;
extern const ImageResource GD_IMG_FLAG_PT_PNG;
extern const ImageResource GD_IMG_FLAG_RU_PNG;
extern const ImageResource GD_IMG_GAME_ICON_PNG;
extern const ImageResource GD_IMG_ICON_NOT_FOUND_PNG;
extern const ImageResource GD_IMG_LANGUAGE_ICON_PNG;
extern const ImageResource GD_IMG_MENU_ICON_PNG;
extern const ImageResource GD_IMG_STATS_ICON_PNG;

#endif // GAMEDIARY_IMAGE_RESOURCES_H
