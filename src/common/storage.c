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
 * @file storage.c
 * @brief Storage implementation.
 */

#include "common/storage.h"
#include "common/db_schema.h"
#include "common/utils.h"
#if defined(GDIARY_PLUGIN) && !defined(GDIARY_APP)
#include "plugin/icon_capture.h"
#endif

#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define CACHE_SIZE 256
#define CHUNK_SIZE 128

static GameEntry           g_cache[CACHE_SIZE];
static u32                 g_cache_count = 0;
static GameRegistryHeader  g_header;
static int                 g_initialized = 0;

/* Shared I/O buffer — static to avoid stack overflow in small kernel threads */
static GameEntry g_io_chunk[CHUNK_SIZE];

/* Base directory and db directory cached at storage_init() time.
 * Avoids repeated calls to path-resolution logic at runtime. */
static char g_base_dir[128] = {0};
static char g_db_dir[160]   = {0};
static char g_device[8]     = {0};  /* e.g. "ms0:" or "ef0:" */

static void get_full_path(char *out, const char *filename) {
  snprintf(out, 256, "%s/%s", g_db_dir, filename);
}

static void ensure_dir(const char *path) {
  char tmp[128];
  char *p = NULL;
  size_t len;
  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') tmp[len - 1] = 0;
  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      sceIoMkdir(tmp, 0777);
      *p = '/';
    }
  }
  sceIoMkdir(tmp, 0777);
}

static u32 calculate_generic_checksum(const void *data, size_t len, size_t skip_offset) {
    u32 hash = 2166136261U; // FNV offset basis
    const unsigned char *p = (const unsigned char *)data;

    // Safety: ensure we don't hash past the intended header size
    for (size_t i = 0; i < len; i++) {
        // Skip the 4-byte checksum field dynamically
        if (i >= skip_offset && i < skip_offset + sizeof(u32)) continue;

        hash ^= p[i];
        hash *= 16777619U; // FNV prime
    }
    return hash;
}

// Forward-compatibility: Map each version to its expected header size
static size_t get_header_size_by_version(u32 version) {
    if (version <= 3) return 32; // Version up to 3: 32 bytes
    // Future expansion example: if (version == 4) return 64;
    return sizeof(GameRegistryHeader); // Fallback to current size
}

static int validate_header(const GameRegistryHeader *h) {
    if (h->magic != GAMEDIARY_MAGIC) return -1;
    if (h->version > DB_VERSION) return -2; // Cannot read "future" versions
    if (h->ready_flag != GAMEDIARY_MAGIC) return -3;

    // Use the size that was relevant for THIS header's version
    size_t hash_len = get_header_size_by_version(h->version);
    u32 expected_hash = calculate_generic_checksum(h, hash_len, offsetof(GameRegistryHeader, checksum));

    if (h->checksum != expected_hash) return -4;
    return 0;
}

static int load_reliable_header(GameRegistryHeader *header, const char *path) {
    SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (fd < 0) return -1;

    // 1. Try Primary Header
    int res = sceIoRead(fd, header, sizeof(GameRegistryHeader));
    if (res == sizeof(GameRegistryHeader) && validate_header(header) == 0) {
        sceIoClose(fd);
        return 0;
    }

    // 2. Try Backup Header at the end
    SceOff size = sceIoLseek(fd, 0, PSP_SEEK_END);
    if (size > (SceOff)sizeof(GameRegistryHeader)) {
        sceIoLseek(fd, size - sizeof(GameRegistryHeader), PSP_SEEK_SET);
        res = sceIoRead(fd, header, sizeof(GameRegistryHeader));
        if (res == sizeof(GameRegistryHeader) && validate_header(header) == 0) {
            sceIoClose(fd);
            return 0;
        }
    }

    sceIoClose(fd);
    return -2;
}

static int save_registry_atomic(const GameRegistryHeader *h_template, const GameEntry *new_entry) {
    char path[256], tmp_path[256];
    get_full_path(path, GAMES_DAT);
    get_full_path(tmp_path, GAMES_TMP);

    SceUID out_fd = sceIoOpen(tmp_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (out_fd < 0) return -1;

    GameRegistryHeader h = *h_template;
    h.ready_flag = 0; // Not ready yet
    h.checksum = 0;

    // 1. Write initial header
    sceIoWrite(out_fd, &h, sizeof(GameRegistryHeader));

    // 2. Copy existing entries
    SceUID in_fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
    if (in_fd >= 0) {
        sceIoLseek(in_fd, sizeof(GameRegistryHeader), PSP_SEEK_SET);
        u32 to_copy = h_template->num_entries; // Original count
        u32 copied = 0;
        while (copied < to_copy) {
            u32 n = (to_copy - copied > CHUNK_SIZE) ? CHUNK_SIZE : (to_copy - copied);
            int r = sceIoRead(in_fd, g_io_chunk, n * sizeof(GameEntry));
            if (r <= 0) break;
            sceIoWrite(out_fd, g_io_chunk, r);
            copied += (r / sizeof(GameEntry));
        }
        sceIoClose(in_fd);
    }

    // 3. Write new entry
    if (new_entry) {
        sceIoWrite(out_fd, new_entry, sizeof(GameEntry));
        h.num_entries++;
    }

    // 4. Update Header and Backup
    h.version = DB_VERSION;
    h.ready_flag = GAMEDIARY_MAGIC;
    h.checksum = calculate_generic_checksum(&h, get_header_size_by_version(h.version), offsetof(GameRegistryHeader, checksum));

    sceIoLseek(out_fd, 0, PSP_SEEK_SET);
    sceIoWrite(out_fd, &h, sizeof(GameRegistryHeader)); // Update primary

    sceIoLseek(out_fd, 0, PSP_SEEK_END);
    sceIoWrite(out_fd, &h, sizeof(GameRegistryHeader)); // Write backup at end

    // 5. Cleanup and Rename
    sceIoClose(out_fd);

    sceIoRemove(path);
    int res = sceIoRename(tmp_path, path);
    if (res >= 0) {
        sceIoSync(g_device, 0); /* One critical sync after the atomic rename */
    }

    return (res >= 0) ? 0 : -2;
}

void storage_init(const char *base_dir) {
  if (g_initialized) return;

  /* Store the base dir and derive the db sub-directory and device prefix.
   * The caller (plugin/main.c) is responsible for resolving which device
   * to use (sctrlKernelMsIsEf), keeping this layer kernel-agnostic. */
  snprintf(g_base_dir, sizeof(g_base_dir), "%s", base_dir);
  snprintf(g_db_dir,   sizeof(g_db_dir),   "%s/db", base_dir);

  /* Extract device prefix (e.g. "ms0:" or "ef0:") from the base path. */
  const char *colon = strchr(base_dir, ':');
  if (colon && (colon - base_dir + 1) < (int)sizeof(g_device)) {
    int len = (int)(colon - base_dir + 1);
    snprintf(g_device, sizeof(g_device), "%.*s", len, base_dir);
  } else {
    snprintf(g_device, sizeof(g_device), "ms0:");
  }

  ensure_dir(g_base_dir);
  ensure_dir(g_db_dir);

  /* Ensure icons directory exists for capturing game icons. */
  char icons_dir[160];
  snprintf(icons_dir, sizeof(icons_dir), "%s/icons", g_base_dir);
  ensure_dir(icons_dir);

  char path[256], tmp_path[256];
  get_full_path(path, GAMES_DAT);
  get_full_path(tmp_path, GAMES_TMP);

  /* Recovery: check if games.tmp is valid and should be promoted. */
  GameRegistryHeader tmp_header;
  if (load_reliable_header(&tmp_header, tmp_path) == 0) {
      sceIoRemove(path);
      sceIoRename(tmp_path, path);
      sceIoSync(g_device, 0);
  } else {
      sceIoRemove(tmp_path);
  }

  if (load_reliable_header(&g_header, path) < 0) {
    g_header.magic = GAMEDIARY_MAGIC;
    g_header.version = DB_VERSION;
    g_header.num_entries = 0;
    g_header.next_uid = 1;
    g_header.ready_flag = GAMEDIARY_MAGIC;
    // Use the version-aware size for initial creation as well
    g_header.checksum = calculate_generic_checksum(&g_header, get_header_size_by_version(g_header.version), offsetof(GameRegistryHeader, checksum));
    memset(g_header.reserved, 0, sizeof(g_header.reserved));

    SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
      sceIoWrite(fd, &g_header, sizeof(GameRegistryHeader));
      sceIoClose(fd);
    }
  }

  // Session log integrity check
  char s_path[256];
  get_full_path(s_path, SESSIONS_DAT);
  SceUID s_fd = sceIoOpen(s_path, PSP_O_RDONLY, 0777);
  if (s_fd >= 0) {
    SceOff size = sceIoLseek(s_fd, 0, PSP_SEEK_END);
    sceIoClose(s_fd);
    if (size % sizeof(SessionEntry) != 0) {
      SceOff valid_size = (size / sizeof(SessionEntry)) * sizeof(SessionEntry);
      char s_tmp[256];
      get_full_path(s_tmp, "sessions.tmp");
      SceUID in_fd = sceIoOpen(s_path, PSP_O_RDONLY, 0777);
      SceUID out_fd = sceIoOpen(s_tmp, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
      if (in_fd >= 0 && out_fd >= 0) {
          static char recovery_buf[512]; // Static to avoid stack usage
          SceOff copied = 0;
          while (copied < valid_size) {
              u32 to_read = (valid_size - copied > sizeof(recovery_buf)) ? sizeof(recovery_buf) : (u32)(valid_size - copied);
              int r = sceIoRead(in_fd, recovery_buf, to_read);
              if (r <= 0) break;
              sceIoWrite(out_fd, recovery_buf, r);
              copied += r;
          }
      }
      if (in_fd >= 0) sceIoClose(in_fd);
      if (out_fd >= 0) sceIoClose(out_fd);
      sceIoRemove(s_path);
      sceIoRename(s_tmp, s_path);
      sceIoSync(g_device, 0);
    }
  }

  g_cache_count = 0;
  g_initialized = 1;
}

static void add_to_cache(const GameEntry *entry) {
    u32 count = (g_cache_count < CACHE_SIZE) ? g_cache_count : (CACHE_SIZE - 1);
    memmove(&g_cache[1], &g_cache[0], count * sizeof(GameEntry));
    memcpy(&g_cache[0], entry, sizeof(GameEntry));
    if (g_cache_count < CACHE_SIZE) g_cache_count++;
}

int storage_get_or_create_game(const GameMetadata *meta, u32 *uid) {
  /* Caller must have called storage_init() before this point.
   * If not initialized, we can't recover gracefully without a base_dir. */
  if (!g_initialized) return -1;

  for (u32 i = 0; i < g_cache_count; i++) {
    if (strcmp(g_cache[i].game_id, meta->game_id) == 0) {
      *uid = g_cache[i].uid;
      if (i > 0) {
          GameEntry tmp = g_cache[i];
          memmove(&g_cache[1], &g_cache[0], i * sizeof(GameEntry));
          g_cache[0] = tmp;
      }
      return 0;
    }
  }

  char path[256];
  get_full_path(path, GAMES_DAT);
  SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if (fd >= 0) {
    sceIoLseek(fd, sizeof(GameRegistryHeader), PSP_SEEK_SET);
    int read_bytes;
    while ((read_bytes = sceIoRead(fd, g_io_chunk, sizeof(g_io_chunk))) > 0) {
      u32 entries_in_chunk = read_bytes / sizeof(GameEntry);
      for (u32 i = 0; i < entries_in_chunk; i++) {
        if (strcmp(g_io_chunk[i].game_id, meta->game_id) == 0) {
          *uid = g_io_chunk[i].uid;
          add_to_cache(&g_io_chunk[i]);
          sceIoClose(fd);
          return 0;
        }
      }
    }
    sceIoClose(fd);
  }

  GameEntry new_game;
  memset(&new_game, 0, sizeof(GameEntry));
  new_game.uid = g_header.next_uid++;

  // Safe string copying with manual null termination assurance
  snprintf(new_game.game_id, sizeof(new_game.game_id), "%s", meta->game_id);
  snprintf(new_game.game_name, sizeof(new_game.game_name), "%s", meta->game_name);
  snprintf(new_game.apitype_str, sizeof(new_game.apitype_str), "%s", meta->apitype_str);

  new_game.category = meta->category;

  if (save_registry_atomic(&g_header, &new_game) == 0) {
      g_header.num_entries++;
      *uid = new_game.uid;
      add_to_cache(&new_game); // Success! Add to cache now.

#if defined(GDIARY_PLUGIN) && !defined(GDIARY_APP)
      /* Capture icon using the stored base dir. */
      char icons_dir[160];
      snprintf(icons_dir, sizeof(icons_dir), "%s/icons", g_base_dir);
      utils_capture_icon(new_game.game_id, new_game.category, icons_dir, meta->file_path);
#endif

      return 0;
  }

  return -1;
}

int storage_log_session(u32 game_uid, u32 duration, u32 timestamp, SceOff *offset) {
  if (!g_initialized) return -1;  /* Must call storage_init() first */
  char path[256];
  get_full_path(path, SESSIONS_DAT);

  SceUID fd;
  SessionEntry entry;
  entry.game_uid = game_uid;
  entry.duration = duration;
  entry.timestamp = timestamp;

  if (*offset == -1) {
    // New session: Append
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
    if (fd < 0) return -1;

    // Get the current offset before writing (since it's append, it's just size)
    *offset = sceIoLseek(fd, 0, PSP_SEEK_END);
  } else {
    // Existing session: Overwrite
    fd = sceIoOpen(path, PSP_O_WRONLY, 0777);
    if (fd < 0) return -2;
    sceIoLseek(fd, *offset, PSP_SEEK_SET);
  }

  int res = sceIoWrite(fd, &entry, sizeof(SessionEntry));
  sceIoClose(fd);
  return (res == sizeof(SessionEntry)) ? 0 : -3;
}
