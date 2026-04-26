/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/audio/audio_manager.h"
#include "app/config/config.h"
#include <pspaudio.h>
#include <pspkernel.h>
#include <string.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define MAX_SAMPLES (44100 * 250 / 1000) // Up to 250ms
#define SFX_QUEUE_SIZE 8

typedef enum {
    WAVE_SINE,
    WAVE_SQUARE
} WaveType;

static short s_buffers[SFX_COUNT][MAX_SAMPLES * 2];
static int   s_lengths[SFX_COUNT];

static SoundID s_queue[SFX_QUEUE_SIZE];
static int s_q_head = 0;
static int s_q_tail = 0;
static int s_q_count = 0;

static SceUID s_thread_id = -1;
static SceUID s_sema_event = -1;
static SceUID s_sema_mutex = -1;
static int s_run_audio = 1;

static void generate_tone(SoundID id, WaveType type, float freq_start, float freq_end, int duration_ms, float volume, int add_harmonics) {
    int total_samples = (duration_ms * SAMPLE_RATE) / 1000;
    if (total_samples > MAX_SAMPLES) total_samples = MAX_SAMPLES;
    
    s_lengths[id] = total_samples;
    short* out_ptr = s_buffers[id];
    
    float phase = 0.0f;
    for (int i = 0; i < total_samples; i++) {
        float progress = (float)i / total_samples;
        
        // Pitch interpolation
        float current_freq = freq_start + (freq_end - freq_start) * progress;
        
        // Phase accumulator
        phase += 2.0f * M_PI * current_freq / SAMPLE_RATE;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        float sample = 0.0f;
        if (type == WAVE_SINE) {
            sample = sinf(phase);
            if (add_harmonics) {
                sample += 0.3f * sinf(phase * 2.0f);
                sample /= 1.3f;
            }
            // Light distortion for some character (optional, requested on cancel)
            if (freq_start > 500 && freq_end < 600) {
               // Pseudo-wave folding
               if (sample > 0.8f) sample = 0.8f;
               if (sample < -0.8f) sample = -0.8f;
            }
        } else if (type == WAVE_SQUARE) {
            sample = sinf(phase) > 0.0f ? 1.0f : -1.0f;
        }
        
        // Envelope: Attack / Decay
        float env = 1.0f;
        if (progress > 0.15f) {
            // Decay
            float decay_prog = (progress - 0.15f) / 0.85f;
            env = expf(-decay_prog * 5.0f); // Fast exponential decay
        } else {
            // Attack
            env = progress / 0.15f; 
        }
        
        short val = (short)(sample * env * volume * 32767.0f);
        *out_ptr++ = val; // Left
        *out_ptr++ = val; // Right
    }
}

static int audio_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    
    int channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_STEREO);
    
    while (s_run_audio) {
        sceKernelWaitSema(s_sema_event, 1, 0); // Wait for an SFX
        if (!s_run_audio) break;
        
        sceKernelWaitSema(s_sema_mutex, 1, 0);
        if (s_q_count == 0) {
            sceKernelSignalSema(s_sema_mutex, 1);
            continue;
        }
        SoundID id = s_queue[s_q_head];
        s_q_head = (s_q_head + 1) % SFX_QUEUE_SIZE;
        s_q_count--;
        sceKernelSignalSema(s_sema_mutex, 1);
        
        if (id >= SFX_COUNT) continue;
        
        int total_samples = s_lengths[id];
        short* snd_buf = s_buffers[id];
        int chunk_size = 512;
        
        for (int i = 0; i < total_samples; i += chunk_size) {
            if (!s_run_audio) break;
            
            // Allow interrupting long sounds if queue is filling up fast? 
            // For ~100ms sounds, overlapping/interrupting isn't strictly necessary.
            
            int to_copy = total_samples - i;
            if (to_copy > chunk_size) to_copy = chunk_size;
            
            short temp[512 * 2];
            memset(temp, 0, sizeof(temp));
            memcpy(temp, &snd_buf[i * 2], to_copy * 2 * sizeof(short));
            
            sceAudioOutputPannedBlocking(channel, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, temp);
        }
    }
    
    if (channel >= 0) sceAudioChRelease(channel);
    return 0;
}

void audio_init(void) {
    // Navigate: Sharp square click (40ms, 3kHz)
    generate_tone(SFX_NAVIGATE, WAVE_SQUARE, 3000.0f, 3000.0f, 40,  0.3f, 0);
    // Confirm: Smooth harmonic ding (100ms, 1200Hz)
    generate_tone(SFX_CONFIRM,  WAVE_SINE,   1200.0f, 1200.0f, 100, 0.5f, 1);
    // Cancel: Downward pitched thud (100ms, 700Hz -> 400Hz)
    generate_tone(SFX_CANCEL,   WAVE_SINE,   700.0f,  400.0f,  100, 0.5f, 1);

    s_sema_event = sceKernelCreateSema("AudioEvent", 0, 0, SFX_QUEUE_SIZE, 0);
    s_sema_mutex = sceKernelCreateSema("AudioMutex", 0, 1, 1, 0);
    
    s_run_audio = 1;
    s_thread_id = sceKernelCreateThread("GameDiarySFX", audio_thread, 0x18, 0x1000, 0, 0);
    if (s_thread_id >= 0) {
        sceKernelStartThread(s_thread_id, 0, NULL);
    }
}

void audio_cleanup(void) {
    s_run_audio = 0;
    if (s_sema_event >= 0) sceKernelSignalSema(s_sema_event, 1);
    if (s_thread_id >= 0) sceKernelWaitThreadEnd(s_thread_id, NULL);
    if (s_sema_event >= 0) sceKernelDeleteSema(s_sema_event);
    if (s_sema_mutex >= 0) sceKernelDeleteSema(s_sema_mutex);
}

void audio_play_sfx(SoundID id) {
    AppConfig* cfg = config_get();
    if (!cfg || !cfg->sfx_enabled) return;
    if (s_sema_mutex < 0) return; // not initialized
    
    sceKernelWaitSema(s_sema_mutex, 1, 0);
    if (s_q_count < SFX_QUEUE_SIZE) {
        s_queue[s_q_tail] = id;
        s_q_tail = (s_q_tail + 1) % SFX_QUEUE_SIZE;
        s_q_count++;
        sceKernelSignalSema(s_sema_event, 1);
    }
    sceKernelSignalSema(s_sema_mutex, 1);
}
