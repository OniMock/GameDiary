#ifndef _DB_SCHEMA_H_
#define _DB_SCHEMA_H_

#include <pspkernel.h>

// Magic "GAMD"
#define GAMEDIARY_MAGIC 0x444D4147
#define DB_VERSION 2

#define DB_PATH "ms0:/PSP/COMMON/GameDiary/diary.dat"
#define DB_TMP_PATH "ms0:/PSP/COMMON/GameDiary/diary.tmp"
#define DB_DIR "ms0:/PSP/COMMON/GameDiary"

// EF0 fallback
#define DB_PATH_EF0 "ef0:/PSP/COMMON/GameDiary/diary.dat"
#define DB_TMP_PATH_EF0 "ef0:/PSP/COMMON/GameDiary/diary.tmp"
#define DB_DIR_EF0 "ef0:/PSP/COMMON/GameDiary"

typedef struct {
  char game_id[16];    // e.g. "ULUS-10041"
  char game_name[64];  // e.g. "Grand Theft Auto: Liberty City Stories"
  char apitype_str[8]; // e.g. "0x120"
  u32 total_time;      // in seconds
  u32 first_played;    // UNIX timestamp
  u32 last_played;     // UNIX timestamp
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

#endif // _DB_SCHEMA_H_
