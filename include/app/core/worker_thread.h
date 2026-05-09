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
 * @file worker_thread.h
 * @brief Centralized worker thread for heavy operations (I/O, PNG loading, config save).
 */

#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <psptypes.h>
#include "app/config/config.h"
#include "app/ui/carousel_state.h"

/**
 * @brief Initializes the worker thread and its task queue.
 */
void worker_thread_init(void);

/**
 * @brief Gracefully shuts down the worker thread, waiting for pending tasks to complete or drop.
 */
void worker_thread_shutdown(void);

/**
 * @brief Asynchronously saves the application configuration.
 *
 * @param cfg Snapshot of the configuration to save.
 * @return 0 if enqueued successfully, < 0 on error (e.g., queue full).
 */
int worker_enqueue_save_config(const AppConfig* cfg);

/**
 * @brief Asynchronously loads a PNG icon for the carousel.
 *
 * @param cs Pointer to the CarouselState.
 * @param slot_idx The cache slot index in the CarouselState.
 * @param inf_idx The infinite index of the game.
 * @param path The absolute path to the PNG file.
 * @return 0 if enqueued successfully, < 0 on error.
 */
int worker_enqueue_load_icon(CarouselState *cs, int slot_idx, int inf_idx, const char* path);

#endif /* WORKER_THREAD_H */
