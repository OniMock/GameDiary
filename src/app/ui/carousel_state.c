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
 * @brief Lateral carousel logic (no rendering).
 *
 * Cache access is protected by a mutex to avoid race conditions.
 * A loader thread handles icon loading in parallel; if it fails
 * (e.g. PSP 1000), the system falls back to synchronous mode.
 *
 * current_idx is unbounded and normalized using wrap_idx().
 * State is shared between main and loader threads with mutex
 * synchronization and memory barriers.
 */

#include "app/ui/carousel_state.h"
#include "app/data/data_loader.h"
#include "app/render/texture.h"
#include "common/utils.h"
#include "common/db_schema.h"
#include "common/psp_hardware.h"

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspthreadman.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Module-level singletons (one loader thread shared by all CarouselState
 * instances — only one screen is active at a time on the PSP).
 * ----------------------------------------------------------------------- */

/** Background loader thread handle; -1 when not running. */
static SceUID s_loader_thid = -1;

/** Non-zero while the loader thread should keep running. */
static volatile int s_loader_run = 0;

/**
 * Pointer to the currently active carousel state.
 * Written by the main thread under __sync_synchronize(); read by the loader.
 * NULL means the loader should idle.
 */
static volatile CarouselState *s_active_cs = NULL;

/**
 * Mutex protecting every access to cache[] slots.
 * Initialised in carousel_init(), deleted in carousel_destroy().
 * Using LwMutex (Lightweight Mutex) for modern PSP SDK compatibility.
 */
static SceLwMutexWorkarea s_cache_mutex;

/** Flag to track if s_cache_mutex has been created. */
static int s_mutex_init = 0;

/**
 * Synchronous fallback flag.
 * 1 when the loader thread could not be created; carousel_update() then
 * loads at most 1 PNG per frame from the main thread.
 */
static int s_sync_mode = 0;

/* -----------------------------------------------------------------------
 * Sentinel value for uninitialised / evicted cache slots.
 * Must be a value that can never be a valid infinite index during normal
 * operation (inf_idx is bounded to a reasonable navigation range).
 * ----------------------------------------------------------------------- */
#define SLOT_EMPTY_IDX  (-1000000)

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

/**
 * Normalises an infinite (unbounded) index to [0, total).
 * Works correctly for any negative value of inf_idx.
 *
 * @param inf_idx  Unbounded logical index (may be negative).
 * @param total    Total number of items (must be > 0).
 * @return         Physical index in [0, total).
 */
static int wrap_idx(int inf_idx, int total) {
    /* Double-mod guarantees a positive result regardless of C's
     * implementation-defined sign of % for negative operands. */
    return ((inf_idx % total) + total) % total;
}

/**
 * Maps an infinite index to a cache slot index in [0, CAROUSEL_CACHE_SIZE).
 * Robust for negative inf_idx values; CAROUSEL_CACHE_SIZE need not be a
 * power of two.
 *
 * @param inf_idx  Unbounded logical index.
 * @return         Cache slot index in [0, CAROUSEL_CACHE_SIZE).
 */
static int slot_for(int inf_idx) {
    return ((inf_idx % CAROUSEL_CACHE_SIZE) + CAROUSEL_CACHE_SIZE)
           % CAROUSEL_CACHE_SIZE;
}

/**
 * Builds the filesystem path of the icon PNG for a physical game index.
 * The physical index must already be resolved (i.e., normalised + mapped).
 *
 * @param out       Output buffer.
 * @param size      Size of the output buffer.
 * @param phys_idx  Physical index into data_get_games().
 */
static void build_icon_path(char *out, size_t size, int phys_idx) {
    const GameStats *games = data_get_games();
    snprintf(out, size,
             "%s" GDIARY_BASE_DIR "/" GDIARY_ICON_DIR "/%s.png",
             utils_get_device_prefix(),
             games[phys_idx].entry.game_id);
}

/* -----------------------------------------------------------------------
 * Cache synchronisation (main thread only)
 *
 * cache_sync() keeps the cache window aligned with current_idx:
 *   1. Evict every slot whose inf_idx falls outside [lo, hi].
 *   2. Mark every slot inside [lo, hi] that is EMPTY as PENDING so the
 *      loader thread (or sync fallback) knows to load it.
 *
 * ALL cache[] accesses here are protected by s_cache_mutex.
 * ----------------------------------------------------------------------- */

/**
 * Evicts a single cache slot (must be called with s_cache_mutex held).
 * Frees the texture if present and resets the slot to EMPTY sentinel.
 */
static void slot_evict_locked(CarouselState *cs, int s) {
    if (cs->cache[s].tex) {
        texture_free(cs->cache[s].tex);
        cs->cache[s].tex = NULL;
    }
    cs->cache[s].inf_idx = SLOT_EMPTY_IDX;
    cs->cache[s].state   = CACHE_SLOT_EMPTY;
}

/**
 * Synchronises the window [current_idx - RADIUS, current_idx + RADIUS]
 * with the cache: evicts stale entries and schedules missing ones as PENDING.
 *
 * Called from the main thread; acquires the mutex for every cache access.
 */
static void cache_sync(CarouselState *cs) {
    if (cs->total <= 0) return;

    int lo = cs->current_idx - CAROUSEL_CACHE_RADIUS;
    int hi = cs->current_idx + CAROUSEL_CACHE_RADIUS;

    /* -------- Phase 1: evict all slots outside the window ------------- */
    sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        int gi = cs->cache[s].inf_idx;
        if (gi == SLOT_EMPTY_IDX) continue; /* Already empty */
        if (gi < lo || gi > hi) {
            slot_evict_locked(cs, s);
        }
    }
    sceKernelUnlockLwMutex(&s_cache_mutex, 1);

    /* -------- Phase 2: mark missing window slots as PENDING ----------- */
    for (int gi = lo; gi <= hi; gi++) {
        int s = slot_for(gi);

        sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
        if (cs->cache[s].inf_idx == gi) {
            /* Correct game is already in this slot — nothing to do. */
            sceKernelUnlockLwMutex(&s_cache_mutex, 1);
            continue;
        }
        /* Evict any occupant of this slot (may be a different inf_idx). */
        slot_evict_locked(cs, s);

        /* Claim the slot for gi and mark for async/sync load. */
        cs->cache[s].inf_idx = gi;
        cs->cache[s].state   = CACHE_SLOT_PENDING;
        sceKernelUnlockLwMutex(&s_cache_mutex, 1);
    }
}

/* -----------------------------------------------------------------------
 * Background Loader Thread
 *
 * Priority 0x13 (19 decimal) — lower priority number = higher priority on
 * PSP; 0x13 is intentionally LOWER priority than the main loop (0x20 / 32)
 * to prevent icon loading from starving the UI thread.
 *
 * Stack: 0x8000 (32 KB) — libpng needs ~20–24 KB of stack during decode.
 * ----------------------------------------------------------------------- */

static int carousel_loader_thread(SceSize args, void *argp) {
    (void)args; (void)argp;

    while (s_loader_run) {
        /* Memory barrier: ensure we see the latest s_active_cs write. */
        __sync_synchronize();

        CarouselState *cs = (CarouselState *)s_active_cs;
        if (!cs || cs->total <= 0) {
            /* No active carousel — sleep to avoid busy-wait. */
            sceKernelDelayThread(16000); /* 16 ms */
            continue;
        }

        /*
         * Scan the window from centre outward (dist 0 first) looking for
         * a slot in PENDING state.  Load at most 1 PNG per iteration so the
         * main thread is not blocked for more than one decode cycle.
         */
        int loaded_any = 0;

        /* Snapshot current_idx once before scanning — avoids reading a value
        * that the main thread may modify mid-loop (carousel_navigate). */
        __sync_synchronize();
        int cur = cs->current_idx;

        for (int dist = 0; dist <= CAROUSEL_CACHE_RADIUS && !loaded_any; dist++) {
            for (int sign = 1; sign >= -1 && !loaded_any; sign -= 2) {
                int target_inf = cur + dist * sign;
                int s          = slot_for(target_inf);

                /* ---- Check state under mutex ---- */
                sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
                int is_our_slot = (cs->cache[s].inf_idx == target_inf &&
                                   cs->cache[s].state   == CACHE_SLOT_PENDING);
                sceKernelUnlockLwMutex(&s_cache_mutex, 1);

                if (!is_our_slot) {
                    if (dist == 0) break; /* dist==0: only one sign needed */
                    continue;
                }

                /* ---- Resolve physical index (wrap + optional map) ---- */
                int logical  = wrap_idx(target_inf, cs->total);
                int physical = cs->index_map ? cs->index_map[logical] : logical;

                char path[256];
                build_icon_path(path, sizeof(path), physical);

                /* ---- Load PNG (I/O outside the mutex) ---- */
                Texture *tex = texture_load_png(path);

                /* ---- Write result under mutex; validate slot is still ours ---- */
                sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
                if (cs->cache[s].inf_idx == target_inf &&
                    cs->cache[s].state   == CACHE_SLOT_PENDING) {
                    cs->cache[s].tex   = tex;
                    cs->cache[s].state = CACHE_SLOT_LOADED;
                } else {
                    /* Slot was reassigned while we were loading — discard. */
                    if (tex) texture_free(tex);
                }
                sceKernelUnlockLwMutex(&s_cache_mutex, 1);

                loaded_any = 1;

                if (dist == 0) break; /* dist==0: sign loop is irrelevant */
            }
        }

        /* Yield between loads to keep the UI responsive. */
        sceKernelDelayThread(loaded_any ? 2000 : 16000); /* 2 ms / 16 ms */
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * Navigation (public, so screen modules can call it directly if needed)
 * ----------------------------------------------------------------------- */

/**
 * Moves the carousel selection by delta (+1 right, -1 left).
 *
 * Design: current_idx is NOT clamped — it grows/shrinks freely as an
 * infinite integer.  wrap_idx() is applied only when resolving which
 * physical game to display or load.  This keeps the wrap logic in one
 * place and avoids any edge-case at list boundaries.
 */
void carousel_navigate(CarouselState *cs, int delta) {
    if (cs->total <= 1) return; /* Single game or empty — nothing to navigate */

    /* Pre-set the offset opposite to the direction so icons animate toward
     * their new positions (they appear to fly in from outside the screen). */
    cs->anim_offset -= (float)delta;
    cs->current_idx += delta;

    /* Synchronise cache window with new current_idx. */
    cache_sync(cs);
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void carousel_init(CarouselState *cs, int total_games, const int *index_map) {
    /* -------- Create the cache mutex if it doesn't exist yet --------- */
    if (!s_mutex_init) {
        int res = sceKernelCreateLwMutex(&s_cache_mutex, "carousel_cache", 0, 0, NULL);
        if (res == 0) {
            s_mutex_init = 1;
        } else {
            s_sync_mode = 1;
        }
    }

    /*
     * SAFE RE-INITIALIZATION:
     * If this state is already active or contains textures, we must free
     * them before resetting to avoid leaking RAM on PSP 1000.
     */
    if (s_mutex_init) sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);

    /* Disconnect loader from this state temporarily while we wipe it. */
    if (s_active_cs == cs) {
        s_active_cs = NULL;
        __sync_synchronize();
    }

    /* Free any previous content to avoid memory leaks. */
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        if (cs->cache[s].tex) {
            texture_free(cs->cache[s].tex);
            cs->cache[s].tex = NULL;
        }
    }

    memset(cs, 0, sizeof(*cs));
    cs->total       = total_games;
    cs->index_map   = index_map;
    cs->current_idx = 0;
    cs->anim_offset = 0.0f;

    /* Initialise all cache slots to the empty sentinel. */
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        cs->cache[s].inf_idx = SLOT_EMPTY_IDX;
        cs->cache[s].state   = CACHE_SLOT_EMPTY;
    }

    if (s_mutex_init) sceKernelUnlockLwMutex(&s_cache_mutex, 1);

    /* -------- Register active carousel (barrier before the write) ----- */
    __sync_synchronize();
    s_active_cs = cs;
    __sync_synchronize();

    /* -------- Spin up the loader thread if not already running -------- */
    if (!s_sync_mode && s_loader_thid < 0) {
        s_loader_run  = 1;
        int stack = (is_psp_1000()) ? 0x6000 : 0x8000;
        s_loader_thid = sceKernelCreateThread(
            "CarouselLoader",
            carousel_loader_thread,
            0x13,   /* Priority 19: lower than main loop, avoids UI starvation */
            stack, /* 24 KB stack for PSP 1000, 32 KB for others */
            0, NULL
        );

        if (s_loader_thid >= 0) {
            sceKernelStartThread(s_loader_thid, 0, NULL);
            sceKernelDelayThread(1000);
        } else {
            s_loader_thid = -1;
            s_loader_run  = 0;
            s_sync_mode   = 1;
        }
    }

    /* Populate the initial window. */
    if (total_games > 0) cache_sync(cs);
}

void carousel_destroy(CarouselState *cs) {
    /* -------- Tell the loader thread to stop -------- */
    s_loader_run = 0;
    __sync_synchronize();
    s_active_cs = NULL;
    __sync_synchronize();

    /* -------- Wait for the thread to exit, then clean up ------------- */
    if (s_loader_thid >= 0) {
        sceKernelWaitThreadEnd(s_loader_thid, NULL);
        sceKernelDeleteThread(s_loader_thid);
        s_loader_thid = -1;
    }

    /* -------- Free all cached textures (no other thread running now) -- */
    for (int s = 0; s < CAROUSEL_CACHE_SIZE; s++) {
        if (cs->cache[s].tex) {
            texture_free(cs->cache[s].tex);
            cs->cache[s].tex = NULL;
        }
        cs->cache[s].inf_idx = SLOT_EMPTY_IDX;
        cs->cache[s].state   = CACHE_SLOT_EMPTY;
    }
    cs->total = 0;

    /* -------- Delete the mutex (no threads use it anymore) ------------ */
    if (s_mutex_init) {
        sceKernelDeleteLwMutex(&s_cache_mutex);
        s_mutex_init = 0;
    }

    /* Reset sync mode so the next init can try to create a thread again. */
    s_sync_mode = 0;
}

void carousel_update(CarouselState *cs, u32 buttons, u32 pressed) {
    if (cs->total <= 0) return;

    /* ----------------------------------------------------------------
     * Input: detect edge-press and hold state
     * ---------------------------------------------------------------- */
    int dir = 0;

    /* Immediate single-press navigation. */
    if      (pressed & PSP_CTRL_RIGHT) dir = +1;
    else if (pressed & PSP_CTRL_LEFT)  dir = -1;

    /* Track hold direction for fast-scroll acceleration. */
    if      (buttons & PSP_CTRL_RIGHT) cs->hold_dir = +1;
    else if (buttons & PSP_CTRL_LEFT)  cs->hold_dir = -1;
    else                               cs->hold_dir =  0;

    if (cs->hold_dir != 0) {
        cs->hold_frames++;
        if (cs->hold_frames > CAROUSEL_HOLD_THRESHOLD) {
            /* Fast-scroll: fire a navigation event every FAST_STEP frames. */
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
     * Lerp easing — smooth approach to 0.0 without overshoot.
     * ---------------------------------------------------------------- */
    cs->anim_offset += (0.0f - cs->anim_offset) * CAROUSEL_EASE_FACA;

    /* Snap to rest position to eliminate sub-pixel jitter. */
    if (fabsf(cs->anim_offset) < 0.005f) {
        cs->anim_offset = 0.0f;
    }

    /* ----------------------------------------------------------------
     * Synchronous fallback: load at most 1 pending PNG per frame from
     * the main thread when the loader thread is unavailable.
     * ---------------------------------------------------------------- */
    if (s_sync_mode) {
        int lo = cs->current_idx - CAROUSEL_CACHE_RADIUS;
        int hi = cs->current_idx + CAROUSEL_CACHE_RADIUS;

        for (int gi = lo; gi <= hi; gi++) {
            int s = slot_for(gi);

            /* Check under mutex to avoid tearing (even in sync mode a
             * previous frame may have started filling this slot). */
            if (s_mutex_init) sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);

            int needs_load = (cs->cache[s].inf_idx == gi &&
                              cs->cache[s].state   == CACHE_SLOT_PENDING);

            if (s_mutex_init) sceKernelUnlockLwMutex(&s_cache_mutex, 1);

            if (!needs_load) continue;

            /* Resolve physical index using wrap and optional map. */
            int logical  = wrap_idx(gi, cs->total);
            int physical = cs->index_map ? cs->index_map[logical] : logical;

            char path[256];
            build_icon_path(path, sizeof(path), physical);

            Texture *tex = texture_load_png(path);

            /* Write result under mutex. */
            if (s_mutex_init) sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
            if (cs->cache[s].inf_idx == gi &&
                cs->cache[s].state   == CACHE_SLOT_PENDING) {
                cs->cache[s].tex   = tex;
                cs->cache[s].state = CACHE_SLOT_LOADED;
            } else {
                if (tex) texture_free(tex);
            }
            if (s_mutex_init) sceKernelUnlockLwMutex(&s_cache_mutex, 1);

            /* Load only 1 PNG per frame to keep the UI from freezing. */
            break;
        }
    }
}

Texture *carousel_get_icon(const CarouselState *cs, int inf_idx) {
    if (cs->total <= 0) return NULL;

    int s = slot_for(inf_idx);
    Texture *tex = NULL;

    sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);
    if (cs->cache[s].inf_idx == inf_idx &&
        cs->cache[s].state   == CACHE_SLOT_LOADED) {
        tex = cs->cache[s].tex;
    }
    sceKernelUnlockLwMutex(&s_cache_mutex, 1);

    return tex;
}

CacheSlotState carousel_get_icon_state(const CarouselState *cs, int inf_idx) {
    if (cs->total <= 0) return CACHE_SLOT_EMPTY;

    int s = slot_for(inf_idx);
    CacheSlotState state = CACHE_SLOT_EMPTY;

    sceKernelLockLwMutex(&s_cache_mutex, 1, NULL);

    if (cs->cache[s].inf_idx == inf_idx) {
        state = cs->cache[s].state;
    }
    sceKernelUnlockLwMutex(&s_cache_mutex, 1);

    return state;
}

void carousel_set_index(CarouselState *cs, int index) {
    if (cs->total <= 0) return;
    /* Caller must pass a normalised index in [0, total). */
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
        if (!found) {
            if (unique < MAX_TRACKED_DAYS) {
                day_ids[unique++] = day;
            } else {
                /* Overflow: return -1 to signal the caller */
                return -1;
            }
        }
    }
#undef MAX_TRACKED_DAYS
    return unique;
}
