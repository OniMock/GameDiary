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
 * @file tracker_db.c
 * @brief Blocked-game list I/O (incremental, no malloc).
 */

#include "plugin/tracker_db.h"
#include "common/db_schema.h"
#include "common/utils.h"
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define TRACKER_IO_CHUNK  1024
#define TRACKER_IO_YIELD_US 1000

static char g_tracker_path[160];

/* Shared scratch — avoids large stack in kernel thread */
static u8 g_tracker_chunk[TRACKER_IO_CHUNK];

static void io_yield(void) {
  sceKernelDelayThread(TRACKER_IO_YIELD_US);
}

void tracker_db_set_path(const char *tracker_dat_path) {
  if (!tracker_dat_path) {
    g_tracker_path[0] = '\0';
    return;
  }
  snprintf(g_tracker_path, sizeof(g_tracker_path), "%s", tracker_dat_path);
  g_tracker_path[sizeof(g_tracker_path) - 1] = '\0';
}

static int path_is_set(void) {
  return g_tracker_path[0] != '\0';
}

static int validate_header(const TrackerHeader *h) {
  if (!h || h->magic != TRACKER_MAGIC || h->version != TRACKER_DB_VERSION) {
    return 0;
  }
  return 1;
}

static int game_id_valid(const char *game_id) {
  return game_id && game_id[0] != '\0';
}

static int entry_matches_id(const TrackerEntry *e, const char *game_id) {
  if (!e || !game_id) {
    return 0;
  }
  return strncmp(e->game_id, game_id, TRACKER_GAME_ID_LEN) == 0;
}

static int scan_entries_contains(const char *game_id, u32 data_offset) {
  SceUID fd = sceIoOpen(g_tracker_path, PSP_O_RDONLY, 0777);
  if (fd < 0) {
    return 0;
  }

  if (data_offset > 0) {
    sceIoLseek(fd, (SceOff)data_offset, PSP_SEEK_SET);
  }

  int found = 0;
  for (;;) {
    int n = sceIoRead(fd, g_tracker_chunk, sizeof(g_tracker_chunk));
    if (n <= 0) {
      break;
    }
    io_yield();

    u32 pos = 0;
    while (pos + (int)sizeof(TrackerEntry) <= (u32)n) {
      TrackerEntry *e = (TrackerEntry *)(g_tracker_chunk + pos);
      if (entry_matches_id(e, game_id)) {
        found = 1;
        break;
      }
      pos += sizeof(TrackerEntry);
    }
    if (found) {
      break;
    }

    if (n < (int)sizeof(g_tracker_chunk)) {
      break;
    }
  }

  sceIoClose(fd);
  return found;
}

int tracker_db_contains(const char *game_id) {
  if (!path_is_set() || !game_id_valid(game_id)) {
    return 0;
  }

  SceUID fd = sceIoOpen(g_tracker_path, PSP_O_RDONLY, 0777);
  if (fd < 0) {
    return 0;
  }

  TrackerHeader hdr;
  int n = sceIoRead(fd, &hdr, sizeof(hdr));
  int has_header = (n == (int)sizeof(hdr) && validate_header(&hdr));
  u32 data_offset = 0;

  if (has_header) {
    data_offset = sizeof(TrackerHeader);
    sceIoClose(fd);
    return scan_entries_contains(game_id, data_offset);
  }

  /* Legacy / headerless: entire file is entries */
  sceIoLseek(fd, 0, PSP_SEEK_SET);
  int found = 0;
  for (;;) {
    int rb = sceIoRead(fd, g_tracker_chunk, sizeof(g_tracker_chunk));
    if (rb <= 0) {
      break;
    }
    io_yield();
    u32 pos = 0;
    while (pos + sizeof(TrackerEntry) <= (u32)rb) {
      if (entry_matches_id((TrackerEntry *)(g_tracker_chunk + pos), game_id)) {
        found = 1;
        break;
      }
      pos += sizeof(TrackerEntry);
    }
    if (found || rb < (int)sizeof(g_tracker_chunk)) {
      break;
    }
  }
  sceIoClose(fd);
  return found;
}

static int write_fresh_with_entry(const char *game_id) {
  char tmp_path[168];
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_tracker_path);

  SceUID fd = sceIoOpen(tmp_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (fd < 0) {
    return -1;
  }

  TrackerHeader hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.magic = TRACKER_MAGIC;
  hdr.version = TRACKER_DB_VERSION;
  hdr.entry_count = 1;

  if (sceIoWrite(fd, &hdr, sizeof(hdr)) != (int)sizeof(hdr)) {
    sceIoClose(fd);
    sceIoRemove(tmp_path);
    return -1;
  }

  TrackerEntry ent;
  memset(&ent, 0, sizeof(ent));
  snprintf(ent.game_id, sizeof(ent.game_id), "%s", game_id);

  if (sceIoWrite(fd, &ent, sizeof(ent)) != (int)sizeof(ent)) {
    sceIoClose(fd);
    sceIoRemove(tmp_path);
    return -1;
  }

  sceIoClose(fd);
  sceIoRemove(g_tracker_path);
  sceIoRename(tmp_path, g_tracker_path);
  return 0;
}

static void ensure_parent_dir(void) {
  char dir[160];
  snprintf(dir, sizeof(dir), "%s", g_tracker_path);
  char *slash = strrchr(dir, '/');
  if (slash) {
    *slash = '\0';
    sceIoMkdir(dir, 0777);
  }
}

int tracker_db_add(const char *game_id) {
  if (!path_is_set() || !game_id_valid(game_id)) {
    return -1;
  }

  if (tracker_db_contains(game_id)) {
    return 0;
  }

  ensure_parent_dir();

  SceUID fd = sceIoOpen(g_tracker_path, PSP_O_RDONLY, 0777);
  if (fd < 0) {
    return write_fresh_with_entry(game_id);
  }

  TrackerHeader hdr;
  int hn = sceIoRead(fd, &hdr, sizeof(hdr));
  sceIoClose(fd);

  if (hn != (int)sizeof(hdr) || !validate_header(&hdr)) {
    return write_fresh_with_entry(game_id);
  }

  fd = sceIoOpen(g_tracker_path, PSP_O_WRONLY | PSP_O_APPEND, 0777);
  if (fd < 0) {
    return -1;
  }

  TrackerEntry ent;
  memset(&ent, 0, sizeof(ent));
  snprintf(ent.game_id, sizeof(ent.game_id), "%s", game_id);

  int wr = sceIoWrite(fd, &ent, sizeof(ent));
  sceIoClose(fd);
  if (wr != (int)sizeof(ent)) {
    return -1;
  }

  /* Update entry_count in header */
  fd = sceIoOpen(g_tracker_path, PSP_O_RDWR, 0777);
  if (fd >= 0) {
    hdr.entry_count++;
    sceIoLseek(fd, offsetof(TrackerHeader, entry_count), PSP_SEEK_SET);
    sceIoWrite(fd, &hdr.entry_count, sizeof(hdr.entry_count));
    sceIoClose(fd);
  }

  return 0;
}

int tracker_db_remove(const char *game_id) {
  if (!path_is_set() || !game_id_valid(game_id)) {
    return -1;
  }

  if (!tracker_db_contains(game_id)) {
    return 0;
  }

  char tmp_path[168];
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_tracker_path);

  SceUID in_fd = sceIoOpen(g_tracker_path, PSP_O_RDONLY, 0777);
  if (in_fd < 0) {
    return -1;
  }

  TrackerHeader in_hdr;
  int hn = sceIoRead(in_fd, &in_hdr, sizeof(in_hdr));
  int has_header = (hn == (int)sizeof(in_hdr) && validate_header(&in_hdr));
  if (!has_header) {
    sceIoLseek(in_fd, 0, PSP_SEEK_SET);
  }

  SceUID out_fd = sceIoOpen(tmp_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
  if (out_fd < 0) {
    sceIoClose(in_fd);
    return -1;
  }

  TrackerHeader out_hdr;
  memset(&out_hdr, 0, sizeof(out_hdr));
  out_hdr.magic = TRACKER_MAGIC;
  out_hdr.version = TRACKER_DB_VERSION;
  out_hdr.entry_count = 0;

  if (sceIoWrite(out_fd, &out_hdr, sizeof(out_hdr)) != (int)sizeof(out_hdr)) {
    sceIoClose(in_fd);
    sceIoClose(out_fd);
    sceIoRemove(tmp_path);
    return -1;
  }

  u32 data_start = has_header ? sizeof(TrackerHeader) : 0;
  if (data_start > 0) {
    sceIoLseek(in_fd, (SceOff)data_start, PSP_SEEK_SET);
  } else {
    sceIoLseek(in_fd, 0, PSP_SEEK_SET);
  }

  for (;;) {
    int n = sceIoRead(in_fd, g_tracker_chunk, sizeof(g_tracker_chunk));
    if (n <= 0) {
      break;
    }
    io_yield();

    u32 pos = 0;
    while (pos + sizeof(TrackerEntry) <= (u32)n) {
      TrackerEntry *e = (TrackerEntry *)(g_tracker_chunk + pos);
      if (!entry_matches_id(e, game_id)) {
        if (sceIoWrite(out_fd, e, sizeof(TrackerEntry)) == (int)sizeof(TrackerEntry)) {
          out_hdr.entry_count++;
        }
      }
      pos += sizeof(TrackerEntry);
    }

    if (n < (int)sizeof(g_tracker_chunk)) {
      break;
    }
  }

  sceIoClose(in_fd);

  sceIoLseek(out_fd, 0, PSP_SEEK_SET);
  sceIoWrite(out_fd, &out_hdr, sizeof(out_hdr));
  sceIoClose(out_fd);

  sceIoRemove(g_tracker_path);
  sceIoRename(tmp_path, g_tracker_path);
  return 0;
}
