#ifndef _DB_SCHEMA_H_
#define _DB_SCHEMA_H_

#include <pspkernel.h>

// Magic "GAMD"
#define GAMEDIARY_MAGIC 0x444D4147
#define DB_VERSION 3

// Path logic is handled in storage.c with sctrlKernelMsIsEf()
#define DB_DIR_MS "ms0:/PSP/COMMON/GameDiary/db"
#define DB_DIR_EF "ef0:/PSP/COMMON/GameDiary/db"

#define GAMES_DAT "games.dat"
#define GAMES_TMP "games.tmp"
#define SESSIONS_DAT "sessions.dat"

#pragma pack(push, 1)

typedef struct {
  u32 uid;
  char game_id[16];    // e.g. "ULUS-10041"
  char game_name[64];  // e.g. "Grand Theft Auto: Liberty City Stories"
  char apitype_str[8]; // e.g. "0x120"
  u8 category;         // 0: PSP, 1: PS1, 2: Homebrew
  u8 reserved[3];
} __attribute__((packed, aligned(4))) GameEntry;

typedef struct {
  u32 game_uid;
  u32 duration;  // in seconds
  u32 timestamp; // UNIX timestamp
} __attribute__((packed, aligned(4))) SessionEntry;

typedef struct {
  u32 magic;
  u32 version;
  u32 num_entries;
  u32 next_uid;
  u32 ready_flag; // Set to GAMEDIARY_MAGIC when file is fully written
  u32 checksum;   // Simple checksum for integrity
  u32 reserved[2];
} __attribute__((packed, aligned(4))) GameRegistryHeader;

#pragma pack(pop)


#endif // _DB_SCHEMA_H_
