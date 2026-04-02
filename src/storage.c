#include "storage.h"
#include "common.h"
#include "tracker.h"

// Paths based on media
static const char *get_db_dir() {
  return sctrlKernelMsIsEf() ? DB_DIR_EF0 : DB_DIR;
}

static const char *get_db_path() {
  return sctrlKernelMsIsEf() ? DB_PATH_EF0 : DB_PATH;
}

static const char *get_db_tmp_path() {
  return sctrlKernelMsIsEf() ? DB_TMP_PATH_EF0 : DB_TMP_PATH;
}

void storage_init(void) {
  sceIoMkdir(sctrlKernelMsIsEf() ? "ef0:/PSP/COMMON" : "ms0:/PSP/COMMON", 0777);
  sceIoMkdir(get_db_dir(), 0777);
}

// Write the database via a temporary file safely
static int write_safe_db(GameDiaryHeader *header, GameDiaryEntry *entries) {
  const char *path = get_db_path();
  const char *tmp_path = get_db_tmp_path();

  SceUID fd =
      sceIoOpen(tmp_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd < 0)
    return -1;

  sceIoWrite(fd, header, sizeof(GameDiaryHeader));
  sceIoWrite(fd, entries, sizeof(GameDiaryEntry) * header->num_entries);
  sceIoClose(fd);

  sceIoRemove(path);
  sceIoRename(tmp_path, path);
  return 0;
}

void storage_update_session(const char *game_id, const char *game_name,
                            u8 category, u32 session_time, int is_new_session) {
  const char *path = get_db_path();
  GameDiaryHeader header;

  // Default Header
  header.magic = GAMEDIARY_MAGIC;
  header.version = DB_VERSION;
  header.num_entries = 0;
  header.reserved = 0;

  // Read existing database to memory (it is small enough, max 500 games * ~90
  // bytes = 45KB) We allocate dynamically in kernel but safely, or we use a
  // static buffer? Let's use a static buffer to ensure absolutely zero memory
  // fragmentation. 500 entries = 46 KB static buffer.
  static GameDiaryEntry g_entries[500];

  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if (fd >= 0) {
    if (sceIoRead(fd, &header, sizeof(GameDiaryHeader)) ==
        sizeof(GameDiaryHeader)) {
      if (header.magic == GAMEDIARY_MAGIC && header.version == DB_VERSION) {
        if (header.num_entries > 500)
          header.num_entries = 500;
        sceIoRead(fd, g_entries, sizeof(GameDiaryEntry) * header.num_entries);
      } else {
        header.num_entries = 0; // Invalid DB, overwrite
      }
    }
    sceIoClose(fd);
  }

  u32 now = get_current_timestamp();
  int found_idx = -1;

  for (u32 i = 0; i < header.num_entries; i++) {
    if (strcmp(g_entries[i].game_id, game_id) == 0) {
      found_idx = i;
      break;
    }
  }

  if (found_idx != -1) {
    // Update existing
    g_entries[found_idx].total_time += session_time;
    g_entries[found_idx].last_played = now;
    if (is_new_session) {
      g_entries[found_idx].session_count++;
    }
    // Always copy name just in case it updated or fixed
    strncpy(g_entries[found_idx].game_name, game_name, 63);
    g_entries[found_idx].category = category;
  } else {
    // Add new
    if (header.num_entries < 500) {
      found_idx = header.num_entries++;
      strncpy(g_entries[found_idx].game_id, game_id, 15);
      g_entries[found_idx].game_id[15] = '\0';
      strncpy(g_entries[found_idx].game_name, game_name, 63);
      g_entries[found_idx].game_name[63] = '\0';
      g_entries[found_idx].total_time = session_time;
      g_entries[found_idx].first_played = now;
      g_entries[found_idx].last_played = now;
      g_entries[found_idx].session_count = 1;
      g_entries[found_idx].category = category;

      for (int i = 0; i < 3; i++)
        g_entries[found_idx].reserved[i] = 0;
    }
  }

  write_safe_db(&header, g_entries);
}
