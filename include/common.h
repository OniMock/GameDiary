#ifndef _COMMON_H_
#define _COMMON_H_

#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <stdio.h>
#include <string.h>

#include "pspsdk/systemctrl.h"

// Magic "GAMD"
#define GAMEDIARY_MAGIC 0x444D4147
#define DB_VERSION 1

#define DB_PATH "ms0:/PSP/COMMON/GameDiary/diary.dat"
#define DB_TMP_PATH "ms0:/PSP/COMMON/GameDiary/diary.tmp"
#define DB_DIR "ms0:/PSP/COMMON/GameDiary"

// EF0 fallback
#define DB_PATH_EF0 "ef0:/PSP/COMMON/GameDiary/diary.dat"
#define DB_TMP_PATH_EF0 "ef0:/PSP/COMMON/GameDiary/diary.tmp"
#define DB_DIR_EF0 "ef0:/PSP/COMMON/GameDiary"

// Categories
#define CAT_PSP 0
#define CAT_PS1 1
#define CAT_HOMEBREW 2
#define CAT_UNKNOWN 3

typedef struct {
  char game_id[16];   // e.g. "ULUS-10041"
  char game_name[64]; // e.g. "Grand Theft Auto: Liberty City Stories"
  u32 total_time;     // in seconds
  u32 first_played;   // UNIX timestamp
  u32 last_played;    // UNIX timestamp
  u32 session_count;
  u8 category; // 0: PSP, 1: PS1, 2: Homebrew
  u8 reserved[3];
} GameDiaryEntry;

typedef struct {
  u32 magic;
  u32 version;
  u32 num_entries;
  u32 reserved;
} GameDiaryHeader;

extern char g_game_id[16];
extern char g_game_name[64];
extern u8 g_category;

// External API functions
void tracker_thread_start(void);
void tracker_thread_stop(void);

void detector_init(void);
void detector_init_late(void);

void storage_init(void);
void storage_update_session(const char *game_id, const char *game_name,
                            u8 category, u32 session_time, int is_new_session);

// Helper to get UNIX timestamp
u32 get_current_timestamp(void);

#endif // _COMMON_H_
