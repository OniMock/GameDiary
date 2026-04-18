/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_CAROUSEL_STATE_H
#define GAMEDIARY_CAROUSEL_STATE_H

/**
 * @file carousel_state.h
 * @brief Domain layer for the lateral icon carousel.
 *
 * Responsibilities:
 *  - Maintain current selection index and spring-easing animation state.
 *  - Manage a sliding window texture cache (no I/O inside the render path).
 *  - Handle fast-scroll acceleration when a directional button is held.
 *
 * This module is pure Domain: it never draws anything directly.
 */

#include <psptypes.h>
#include "app/render/texture.h"
#include "common/db_schema.h"

/* -----------------------------------------------------------------------
 * Tuning constants — adjust for feel without touching logic
 * ----------------------------------------------------------------------- */

/** Number of icon textures kept in RAM simultaneously (must be odd). */
#define CAROUSEL_CACHE_SIZE   7
/** Slots loaded on each side of the centered index. */
#define CAROUSEL_CACHE_RADIUS 3

/** Frames the button must be held before fast-scroll activates. */
#define CAROUSEL_HOLD_THRESHOLD 22
/** Navigation repeat interval (frames) while fast-scrolling. */
#define CAROUSEL_FAST_STEP      6

/** Smooth interpolation factor (Lerp) for easing without ricochet. */
#define CAROUSEL_EASE_FACA    0.15f

/** Pixel distance between adjacent icon centers (used to map offset→px). */
#define CAROUSEL_ICON_STEP  180.0f

/* -----------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------- */

/** State of an icon cache slot for lazy loading. */
typedef enum {
    CACHE_SLOT_EMPTY = 0,
    CACHE_SLOT_PENDING,
    CACHE_SLOT_LOADED
} CacheSlotState;

/** One slot in the sliding-window texture cache. */
typedef struct {
    Texture       *tex;     /**< NULL when slot is empty or load failed.          */
    int            inf_idx; /**< Absolute logical index of game in this slot.     */
    CacheSlotState state;   /**< Current load state of this slot.                 */
} IconCacheSlot;

/**
 * Full domain state for the carousel.
 * Allocate as a static local in the screen module and pass by pointer.
 */
typedef struct {
    /* --- Selection --- */
    int current_idx;   /**< Index of the currently focused game.            */
    int total;         /**< Total number of games; set in carousel_init().   */

    /* --- Lerp animation ---
     * anim_offset: fractional icon displacement in "slot units".
     *   0.0 = settled on current_idx.
     *   Negative values mean icons shifted left (user pressed RIGHT).
     *   Positive values mean icons shifted right (user pressed LEFT). */
    float anim_offset;

    /* --- Hold / fast-scroll --- */
    int hold_dir;    /**< Direction held: +1 right, -1 left, 0 none.        */
    int hold_frames; /**< How many consecutive frames the button is held.    */
    int fast_timer;  /**< Countdown to next fast-scroll fire.                */

    /* --- Icon cache --- */
    IconCacheSlot cache[CAROUSEL_CACHE_SIZE];
} CarouselState;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * Sets the active game index manually (for restoring selection).
 */
void carousel_set_index(CarouselState *cs, int index);

/**
 * Initialises the carousel and pre-loads the icon window.
 * @param cs          State to initialise (caller owns storage).
 * @param total_games Number of games available.
 */
void carousel_init(CarouselState *cs, int total_games);

/**
 * Frees all cached icon textures and resets the state.
 * Call when leaving the screen (destroy callback).
 */
void carousel_destroy(CarouselState *cs);

/**
 * Processes one frame: handles input, advances spring animation,
 * and updates the icon cache as needed.
 * @param cs      Carousel state.
 * @param buttons Currently held button mask (from sceCtrl).
 * @param pressed Buttons that just became pressed this frame.
 */
void carousel_update(CarouselState *cs, u32 buttons, u32 pressed);

/**
 * Returns the cached texture for a given game index, or NULL.
 * Safe to call from the render path — never performs I/O.
 */
Texture *carousel_get_icon(const CarouselState *cs, int game_idx);

/**
 * Returns 1 when the animation has fully settled (offset ≈ 0).
 */
int carousel_is_settled(const CarouselState *cs);

/**
 * Counts unique calendar days (midnight-to-midnight) on which
 * the given game was played, based on session timestamps.
 *
 * @param sessions  Pointer to the global sessions array.
 * @param count     Number of entries in the array.
 * @param game_uid  UID of the game to filter for.
 * @return          Number of distinct days with at least one session.
 */
int carousel_count_days_active(const SessionEntry *sessions, int count,
                               u32 game_uid);

#endif /* GAMEDIARY_CAROUSEL_STATE_H */
