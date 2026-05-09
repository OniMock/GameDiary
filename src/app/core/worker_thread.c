/**
 * -------------------------------------------------------------
 *  GameDiary
 *  Playtime Tracking System for the PlayStation Portable (PSP)
 *
 *  Developed by OniMock
 *  © 2026 OniMock. All rights reserved.
 * -------------------------------------------------------------
 */

#include "app/core/worker_thread.h"
#include "app/render/texture.h"
#include "common/psp_hardware.h"
#include <pspkernel.h>
#include <string.h>

#define WORKER_QUEUE_SIZE 64

typedef enum {
    WORKER_TASK_NONE = 0,
    WORKER_TASK_SAVE_CONFIG,
    WORKER_TASK_LOAD_ICON
} WorkerTaskType;

typedef struct {
    AppConfig config;
} TaskDataSaveConfig;

typedef struct {
    CarouselState *cs;
    int slot_idx;
    int inf_idx;
    char path[128];
} TaskDataLoadIcon;

typedef struct {
    WorkerTaskType type;
    union {
        TaskDataSaveConfig save_config;
        TaskDataLoadIcon load_icon;
    } data;
} WorkerTask;

static WorkerTask s_queue[WORKER_QUEUE_SIZE];
static int s_q_head = 0;
static int s_q_tail = 0;
static int s_q_count = 0;

static SceUID s_thread_id = -1;
static SceUID s_sema_event = -1;
static SceLwMutexWorkarea s_queue_mutex;
static volatile int s_run_worker = 0;

/* External declarations for specific subsystems.
 * This keeps the worker thread decoupled from subsystem internal state. */
extern int config_write_file(const AppConfig* cfg);
extern int carousel_is_slot_pending(CarouselState *cs, int slot_idx, int inf_idx);
extern void carousel_apply_loaded_icon(CarouselState *cs, int slot_idx, int inf_idx, Texture *tex);

static void handle_save_config(const TaskDataSaveConfig *data) {
    config_write_file(&data->config);
}

static void handle_load_icon(const TaskDataLoadIcon *data) {
    /* 1. Check if the slot is still pending before doing expensive I/O */
    if (!carousel_is_slot_pending(data->cs, data->slot_idx, data->inf_idx)) {
        return; // Slot was reassigned or evicted, skip loading.
    }

    /* 2. Load the PNG */
    Texture *tex = texture_load_png(data->path);

    /* 3. Apply the texture. The carousel will double check if the slot is still ours. */
    carousel_apply_loaded_icon(data->cs, data->slot_idx, data->inf_idx, tex);
}

static int worker_thread_main(SceSize args, void *argp) {
    (void)args; (void)argp;

    while (s_run_worker) {
        /* Wait for a task to be enqueued */
        sceKernelWaitSema(s_sema_event, 1, 0);
        if (!s_run_worker) break;

        /* Dequeue task */
        sceKernelLockLwMutex(&s_queue_mutex, 1, NULL);
        if (s_q_count == 0) {
            sceKernelUnlockLwMutex(&s_queue_mutex, 1);
            continue;
        }

        WorkerTask task = s_queue[s_q_head];
        s_q_head = (s_q_head + 1) % WORKER_QUEUE_SIZE;
        s_q_count--;
        sceKernelUnlockLwMutex(&s_queue_mutex, 1);

        /* Process task */
        switch (task.type) {
            case WORKER_TASK_SAVE_CONFIG:
                handle_save_config(&task.data.save_config);
                break;
            case WORKER_TASK_LOAD_ICON:
                handle_load_icon(&task.data.load_icon);
                break;
            default:
                break;
        }

        /* Yield briefly to ensure Audio and UI can run smoothly 
         * if we have multiple heavy tasks back-to-back. */
        sceKernelDelayThread(1000); // 1ms
    }

    return 0;
}

void worker_thread_init(void) {
    if (s_thread_id >= 0) return; // Already initialized

    s_sema_event = sceKernelCreateSema("WorkerEvent", 0, 0, WORKER_QUEUE_SIZE, 0);
    sceKernelCreateLwMutex(&s_queue_mutex, "WorkerQueueMutex", 0, 0, NULL);

    s_q_head = 0;
    s_q_tail = 0;
    s_q_count = 0;
    s_run_worker = 1;

    /* Priority 0x20 (32) is lower than Main Thread (0x18/24) and Audio (0x15/21). 
     * Stack size: 48 KB */
    s_thread_id = sceKernelCreateThread(
        "GameDiaryWorker",
        worker_thread_main,
        0x20,
        48 * 1024,
        0, NULL
    );

    if (s_thread_id >= 0) {
        sceKernelStartThread(s_thread_id, 0, NULL);
    } else {
        s_run_worker = 0;
    }
}

void worker_thread_shutdown(void) {
    s_run_worker = 0;
    
    if (s_sema_event >= 0) {
        /* Wake up the thread if it's sleeping */
        sceKernelSignalSema(s_sema_event, 1);
    }

    if (s_thread_id >= 0) {
        sceKernelWaitThreadEnd(s_thread_id, NULL);
        sceKernelDeleteThread(s_thread_id);
        s_thread_id = -1;
    }

    if (s_sema_event >= 0) {
        sceKernelDeleteSema(s_sema_event);
        s_sema_event = -1;
    }

    sceKernelDeleteLwMutex(&s_queue_mutex);
}

static int enqueue_task(const WorkerTask *task) {
    if (s_thread_id < 0) return -1; // Worker not running

    sceKernelLockLwMutex(&s_queue_mutex, 1, NULL);
    if (s_q_count >= WORKER_QUEUE_SIZE) {
        /* Queue is full, drop task. */
        sceKernelUnlockLwMutex(&s_queue_mutex, 1);
        return -2;
    }

    s_queue[s_q_tail] = *task;
    s_q_tail = (s_q_tail + 1) % WORKER_QUEUE_SIZE;
    s_q_count++;
    sceKernelUnlockLwMutex(&s_queue_mutex, 1);

    sceKernelSignalSema(s_sema_event, 1);
    return 0;
}

int worker_enqueue_save_config(const AppConfig* cfg) {
    WorkerTask task;
    task.type = WORKER_TASK_SAVE_CONFIG;
    task.data.save_config.config = *cfg;
    return enqueue_task(&task);
}

int worker_enqueue_load_icon(CarouselState *cs, int slot_idx, int inf_idx, const char* path) {
    WorkerTask task;
    task.type = WORKER_TASK_LOAD_ICON;
    task.data.load_icon.cs = cs;
    task.data.load_icon.slot_idx = slot_idx;
    task.data.load_icon.inf_idx = inf_idx;
    
    /* Safely copy path */
    strncpy(task.data.load_icon.path, path, sizeof(task.data.load_icon.path) - 1);
    task.data.load_icon.path[sizeof(task.data.load_icon.path) - 1] = '\0';
    
    return enqueue_task(&task);
}
