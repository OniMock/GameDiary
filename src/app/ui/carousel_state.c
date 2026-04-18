/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

/**
 * @file carousel_state.c
 * @brief Domain implementation for the lateral icon carousel.
 *
 * This file contains ONLY logic — no drawing, no GU calls.
 * All I/O (texture_load_png) happens here during carousel_update(),
 * so the render path stays completely I/O-free.
 */

#include "app/ui/carousel_state.h"
#include "app/data/data_loader.h"
#include "app/render/texture.h"

#include <pspctrl.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <pspkernel.h>

/* -----------------------------------------------------------------------
 * Async Background Loader State
 * ----------------------------------------------------------------------- */
static SceUID s_loader_thid = -1;
static volatile int s_loader_run = 0;
static volatile CarouselState *s_active_cs = NULL;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */


/**
 * Direct-mapped cache slot index for a given game_idx.
 * Keeps the mapping stable across small index windows.
 * Always returns a value in [0, CAROUSEL_CACHE_SIZE).
 */
static int slot_for(int game_idx) {
    /* Add CAROUSEL_CACHE_SIZE to guarantee a positive modulo result. */
    return ((game_idx % CAROUSEL_CACHE_SIZE) + CAROUSEL_CACHE_SIZE)
           % CAROUSEL_CACHE_SIZE;
}

/** Writes the icon path for game at game_idx into out[]. */
static void build_icon_path(char *out, size_t size, int game_idx) {
    const GameStats *games = data_get_games();
    snprintf(out, size,
             "ms0:/PSP/COMMON/GameDiary/source/%s.png",
             games[game_idx].entry.game_id);
}

/**
 * Ensures game_idx is loaded into its cache slot.
 * Evicts a different game that currently occupies that slot, if any.
 * Out-of-bounds indices are silently ignored.
 */
static void cache_ensure(CarouselState *cs, int inf_idx) {
    if (cs->total <= 0) return;

    int s = slot_for(inf_idx);

    /* Already the correct game logic — nothing to do. */
    if (cs->cache[s].inf_idx == inf_idx) return;

    /* Evict current occupant before overwriting. */
    if (cs->cache[s].tex) {
        texture_free(cs->cache[s].tex);
        cs->cache[s].tex = NULL;
    }

    /* Mark as pending deferred load safely. */
    cs->cache[s].inf_idx = -1000000;      /* Temporary invalidate */
    cs->cache[s].state   = CACHE_SLOT_EMPTY;
    
    /* Commit new state */
    cs->cache[s].inf_idx = inf_idx;
    cs->cache[s].state   = CACHE_SLOT_PENDING;
}

/* -----------------------------------------------------------------------
 * Background Loader Thread
 * ----------------------------------------------------------------------- */
static int carousel_loader_thread(SceSize args, void *argp) {
    while (s_loader_run) {
        if (!s_active_cs || s_active_cs->total <= 0) {
            sceKernelDelayThread(10000); /* 10ms idle sleep */
            continue;
        }

        int loaded_any = 0;
        for (int dist = 0; dist <= CAROUSEL_CACHE_RADIUS; dist++) {
            for (int sign = 1; sign >= -1; sign -= 2) {
                int target_idx = s_active_cs->current_idx + (dist * sign);
                int s = slot_for(target_idx);

                if (s_active_cs->cache[s].inf_idx == target_idx && 
                    s_active_cs->cache[s].state == CACHE_SLOT_PENDING) {
                    
                    int wrap_idx = (target_idx % s_active_cs->total + s_active_cs->total) % s_active_cs->total;
                    char path[256];
                    build_icon_path(path, sizeof(path), wrap_idx);
                    
                    Texture *tex = texture_load_png(path);
                    
                    /* Validate the slot hasn't been reassigned mid-load */
                    if (s_active_cs->cache[s].inf_idx == target_idx && 
                        s_active_cs->cache[s].state == CACHE_SLOT_PENDING) {
                        s_active_cs->cache[s].tex = tex;
                        s_active_cs->cache[s].state = CACHE_SLOT_LOADED;
                    } else if (tex) {
                        texture_free(tex);
                    }
                    
                    loaded_any = 1;
                    break;
                }
                if (dist == 0) break;
            }
            if (loaded_any) break;
        }

        if (loaded_any) {
            sceKernelDelayThread(1000);  /* 1ms yield to main loop */
        } else {
            sceKernelDelayThread(10000); /* 10ms idle sleep */
        }
    }
    return 0;
}

/**
 * Synchronises the cache with the current window
 * [current_idx - CAROUSEL_CACHE_RADIUS, current_idx + CAROUSEL_CACHE_RADIUS].
 *
 * Steps:
 *   1. Evict any slot whose game_idx falls outside the window.
 *   2. Load any game in the window that is not yet cached.
 */
static void cache_sync(CarouselState *cs) {
    int lo = cs->current_idx - CAROUSEL_CACHE_RADIUS;
    int hi = cs->current_idx + CAROUSEL_CACHE_RADIUS;

    /* 1. Evict stale slots. */
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        int gi = cs->cache[s].inf_idx;
        if (cs->cache[s].inf_idx < -1000000) continue; // Initial state check
        if (gi < lo || gi > hi) {
            if (cs->cache[s].tex) {
                texture_free(cs->cache[s].tex);
                cs->cache[s].tex = NULL;
            }
            cs->cache[s].inf_idx = -1000000;
            cs->cache[s].state   = CACHE_SLOT_EMPTY;
        }
    }

    /* 2. Load missing slots */
    for (int gi = lo; gi <= hi; gi++) {
        cache_ensure(cs, gi);
    }
}

/* -----------------------------------------------------------------------
 * Navigation
 * ----------------------------------------------------------------------- */

/**
 * Moves the selection by delta slots (+1 right, -1 left).
 * Clamps at list edges (no wrap-around — list is ordered by playtime,
 * so wrapping from first to last would be confusing).
 *
 * On navigation the animation offset is pre-set in the OPPOSITE direction
 * so the icons appear to fly toward their new positions.
 */
static void carousel_navigate(CarouselState *cs, int delta) {
    if (cs->total <= 1) return; /* Nothing to navigate */

    /* Shift offset opposite to motion so icons animate toward center. */
    cs->anim_offset -= (float)delta;
    cs->current_idx += delta;
    cache_sync(cs);
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void carousel_init(CarouselState *cs, int total_games) {
    memset(cs, 0, sizeof(*cs));
    cs->total       = total_games;
    cs->current_idx = 0;
    cs->anim_offset  = 0.0f;
    cs->hold_dir     = 0;
    cs->hold_frames  = 0;
    cs->fast_timer   = 0;

    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        cs->cache[s].tex     = NULL;
        cs->cache[s].inf_idx = -1000000;
        cs->cache[s].state   = CACHE_SLOT_EMPTY;
    }

    s_active_cs = cs;
    
    /* Spin up thread if not running */
    if (s_loader_thid < 0) {
        s_loader_run = 1;
        /* Priority 0x18 is slightly lower than normal game loop, preventing complete starvation */
        s_loader_thid = sceKernelCreateThread("GameDiaryIconLoader", carousel_loader_thread,
                                              0x18, 0x4000, 0, NULL);
        if (s_loader_thid >= 0) {
            sceKernelStartThread(s_loader_thid, 0, NULL);
        }
    }

    if (total_games > 0) cache_sync(cs);
}

void carousel_destroy(CarouselState *cs) {
    s_active_cs = NULL; /* Pause loader operations immediately */
    
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        if (cs->cache[s].tex) {
            texture_free(cs->cache[s].tex);
            cs->cache[s].tex = NULL;
        }
        cs->cache[s].inf_idx = -1000000;
        cs->cache[s].state   = CACHE_SLOT_EMPTY;
    }
    cs->total = 0;
}

void carousel_update(CarouselState *cs, u32 buttons, u32 pressed) {
    if (cs->total <= 0) return;

    /* ----------------------------------------------------------------
     * Input: detect edge press and hold state
     * ---------------------------------------------------------------- */
    int dir = 0;

    /* Immediate single-press navigation. */
    if (pressed & PSP_CTRL_RIGHT) dir = +1;
    else if (pressed & PSP_CTRL_LEFT)  dir = -1;

    /* Track hold direction for fast-scroll. */
    if      (buttons & PSP_CTRL_RIGHT) cs->hold_dir = +1;
    else if (buttons & PSP_CTRL_LEFT)  cs->hold_dir = -1;
    else                               cs->hold_dir =  0;

    if (cs->hold_dir != 0) {
        cs->hold_frames++;
        if (cs->hold_frames > CAROUSEL_HOLD_THRESHOLD) {
            /* Fast-scroll: repeat navigation every CAROUSEL_FAST_STEP frames */
            cs->fast_timer--;
            if (cs->fast_timer <= 0) {
                cs->fast_timer = CAROUSEL_FAST_STEP;
                dir = cs->hold_dir;
            }
        }
    } else {
        cs->hold_frames = 0;
        cs->fast_timer  = 0;
    }

    if (dir != 0) carousel_navigate(cs, dir);

    /* ----------------------------------------------------------------
     * Lerp easing
     * Guaranteeing smooth motion without overshoot/ricochet.
     * ---------------------------------------------------------------- */
    cs->anim_offset += (0.0f - cs->anim_offset) * CAROUSEL_EASE_FACA;

    /* Snap to rest to avoid sub-pixel jitter. */
    if (fabsf(cs->anim_offset) < 0.005f) {
        cs->anim_offset = 0.0f;
    }
}

Texture *carousel_get_icon(const CarouselState *cs, int inf_idx) {
    if (cs->total <= 0) return NULL;
    int s = slot_for(inf_idx);
    if (cs->cache[s].inf_idx == inf_idx && cs->cache[s].state == CACHE_SLOT_LOADED) {
        return cs->cache[s].tex;
    }
    return NULL;
}

void carousel_set_index(CarouselState *cs, int index) {
    if (cs->total <= 0) return;
    cs->current_idx = index;
    cs->anim_offset = 0.0f;
    cache_sync(cs);
}

int carousel_is_settled(const CarouselState *cs) {
    return (cs->anim_offset == 0.0f);
}

int carousel_count_days_active(const SessionEntry *sessions, int count,
                               u32 game_uid) {
/* Maximum unique calendar days tracked (avoids VLA / dynamic alloc). */
#define MAX_TRACKED_DAYS 128
    u32 day_ids[MAX_TRACKED_DAYS];
    int unique = 0;

    for (int i = 0; i < count; i++) {
        if (sessions[i].game_uid != game_uid) continue;
        /* Integer division of UNIX timestamp by seconds-per-day gives
         * a unique day identifier regardless of timezone.              */
        u32 day = sessions[i].timestamp / 86400u;

        int found = 0;
        for (int j = 0; j < unique; j++) {
            if (day_ids[j] == day) { found = 1; break; }
        }
        if (!found && unique < MAX_TRACKED_DAYS) day_ids[unique++] = day;
    }
#undef MAX_TRACKED_DAYS
    return unique;
}
