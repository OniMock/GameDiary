/**
 * -------------------------------------------------------------
 *  GameDiary Automated Test Suite
 *  Mock PSP SDK Controller & API Interface
 * -------------------------------------------------------------
 */

#ifndef _MOCK_PSPSDK_H_
#define _MOCK_PSPSDK_H_

#include "../mock_pspsdk/pspkernel.h"
#include "../mock_pspsdk/psprtc.h"
#include "../mock_pspsdk/pspctrl.h"
#include "../mock_pspsdk/pspdisplay.h"
#include "../mock_pspsdk/pspsdk/systemctrl.h"
#include "../mock_pspsdk/pspsdk/kubridge.h"
#include "../mock_pspsdk/psputility.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global lifecycle */
void mock_pspsdk_init(const char *sandbox_dir);
void mock_pspsdk_reset(void);
void mock_pspsdk_cleanup(void);

/* VFS / Storage configuration */
void mock_vfs_set_sandbox_root(const char *sandbox_dir);
const char *mock_vfs_get_sandbox_root(void);
void mock_vfs_map_file(const char *psp_path, const void *data, size_t size);
int  mock_vfs_file_exists(const char *psp_path);
size_t mock_vfs_read_file(const char *psp_path, void *out_buf, size_t max_size);

/* Fault injection */
void mock_vfs_fail_open(bool enable);
void mock_vfs_fail_write(bool enable);
void mock_vfs_fail_rename(bool enable);
void mock_vfs_fail_path(const char *pattern);

/* RTC Control */
void mock_rtc_set_unix_time(u32 seconds);
void mock_rtc_advance_seconds(u32 seconds);
void mock_rtc_set_timezone_offset(int offset_seconds);
u32  mock_rtc_get_unix_time(void);

/* CFW / SystemControl mocks */
void mock_cfw_set_init_param(const char *key, const void *data, u32 len);
void mock_cfw_clear_init_params(void);
void mock_cfw_set_executable_path(const char *path);
void mock_cfw_set_apitype(int apitype);
void mock_cfw_set_umd_file(const char *iso_path);
void mock_cfw_set_umd_stat(int stat);
void mock_cfw_set_hen_version(u32 version);
void mock_cfw_set_devkit_version(int version);

/* Hardware / Memory mocks */
void mock_hardware_set_total_free_mem(u32 bytes);
void mock_hardware_set_system_language(int lang);

/* Input / Pad mocks */
void mock_ctrl_set_buttons(u32 buttons);
u32  mock_ctrl_get_buttons(void);

/* Display mocks */
void mock_display_set_active_fb(void *vram, int stride, int format);
int  mock_display_get_vblank_count(void);

#ifdef __cplusplus
}
#endif

#endif /* _MOCK_PSPSDK_H_ */
