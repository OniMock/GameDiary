/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#ifndef GAMEDIARY_AUDIO_MANAGER_H
#define GAMEDIARY_AUDIO_MANAGER_H

/**
 * @file audio_manager.h
 * @brief SFX playback system using a decoupled dedicated thread.
 */

typedef enum {
    SFX_NAVIGATE,
    SFX_CONFIRM,
    SFX_CANCEL,
    SFX_COUNT
} SoundID;

/**
 * @brief Initializes the audio thread, ring buffer, and statically pre-generates SFX buffers.
 */
void audio_init(void);

/**
 * @brief Unblocks the audio thread and terminates hardware output gracefully.
 */
void audio_cleanup(void);

/**
 * @brief Queues an SFX playback non-blockingly (max 8 per queue).
 * @param id The Sound ID to play
 */
void audio_play_sfx(SoundID id);

#endif // GAMEDIARY_AUDIO_MANAGER_H
