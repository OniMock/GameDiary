#ifndef _SYSTEMCTRL_EPI_H_
#define _SYSTEMCTRL_EPI_H_

#include <psploadcore.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ADRENALINE_TITLEID "PSPEMUCFW"

#define SCE_PSPEMU_FLASH0_PACKAGE_SIZE 0x920000
#define SCE_PSPEMU_EXTRA_MEMORY 0x8B000000
#define SCE_PSPEMU_FLASH0_RAMFS 0x8B000000
#define SCE_PSPEMU_SCRATCHPAD 0x8BD00000
#define SCE_PSPEMU_VRAM 0x8BE00000

#define SCE_PSPEMU_FRAMEBUFFER 0x74000000
#define SCE_PSPEMU_FRAMEBUFFER_SIZE 0x88000

#define PSP_EXTRA_RAM 0x8A000000
#define PSP_EXTRA_RAM_SIZE (32 * 1024 *1024)
#define PSP_RAM_SIZE (64 * 1024 * 1024)

#define PSP_SCREEN_WIDTH 480
#define PSP_SCREEN_HEIGHT 272
#define PSP_SCREEN_LINE 512

#define EPI_CONFIG_ADDR 0x08800010

#define ADRENALINE_SIZE 0x2000
#define ADRENALINE_ADDRESS 0xABCDE000

#define DRAW_NATIVE 0xABCDEF00
#define NATIVE_FRAMEBUFFER 0x0A400000

#define CONVERT_ADDRESS(addr) (0x68000000 + (addr & 0x0FFFFFFF))

enum SaveStateModes {
	SAVESTATE_MODE_NONE,
	SAVESTATE_MODE_SAVE,
	SAVESTATE_MODE_LOAD,
};

enum AdrenalinePspCommands {
	ADRENALINE_PSP_CMD_NONE,
	ADRENALINE_PSP_CMD_REINSERT_MS,
	ADRENALINE_PSP_CMD_SAVESTATE,
	ADRENALINE_PSP_CMD_LOADSTATE,
};

enum AdrenalineVitaCommands {
	ADRENALINE_VITA_CMD_NONE,
	ADRENALINE_VITA_CMD_SAVESTATE,
	ADRENALINE_VITA_CMD_LOADSTATE,
	ADRENALINE_VITA_CMD_GET_USB_STATE,
	ADRENALINE_VITA_CMD_START_USB,
	ADRENALINE_VITA_CMD_STOP_USB,
	ADRENALINE_VITA_CMD_PAUSE_POPS,
	ADRENALINE_VITA_CMD_RESUME_POPS,
	ADRENALINE_VITA_CMD_POWER_SHUTDOWN,
	ADRENALINE_VITA_CMD_POWER_REBOOT,
	ADRENALINE_VITA_CMD_PRINT,
	ADRENALINE_VITA_CMD_UPDATE,
	ADRENALINE_VITA_CMD_APP_STARTED,
	ADRENALINE_VITA_CMD_POWER_TICK,
};

enum AdrenalineVitaResponse {
	ADRENALINE_VITA_RESPONSE_NONE,
	ADRENALINE_VITA_RESPONSE_SAVED,
	ADRENALINE_VITA_RESPONSE_LOADED,
};

typedef struct {
	int savestate_mode;
	int num;
	unsigned int sp;
	unsigned int ra;

	int pops_mode;
	int draw_psp_screen_in_pops;
	char title[128];
	char titleid[12];
	char filename[256];

	int psp_cmd;
	int vita_cmd;
	int psp_response;
	int vita_response;

	char printbuf[1024];
} SceAdrenaline;

enum InfernoCacheConf {
	CACHE_CONFIG_LRU,
    CACHE_CONFIG_RR,
	CACHE_CONFIG_OFF,
};

enum IsoCacheNumberConf {
	ISO_CACHE_NUM_AUTO,
	ISO_CACHE_NUM_001,
	ISO_CACHE_NUM_002,
	ISO_CACHE_NUM_004,
	ISO_CACHE_NUM_008,
	ISO_CACHE_NUM_016,
	ISO_CACHE_NUM_032,
	ISO_CACHE_NUM_064,
	ISO_CACHE_NUM_128,
};

enum IsoCacheSizeConf {
	ISO_CACHE_SIZE_AUTO,
	ISO_CACHE_SIZE_01KB,
	ISO_CACHE_SIZE_02KB,
	ISO_CACHE_SIZE_04KB,
	ISO_CACHE_SIZE_08KB,
	ISO_CACHE_SIZE_16KB,
	ISO_CACHE_SIZE_32KB,
	ISO_CACHE_SIZE_64KB,
};

enum ForceHighMemory {
	HIGHMEM_OPT_OFF,
	HIGHMEM_OPT_STABLE,
	HIGHMEM_OPT_MAX,
};

enum RecoveryColor {
	RECOVERY_COLOR_BLUE,
	RECOVERY_COLOR_GREEN,
	RECOVERY_COLOR_RED,
	RECOVERY_COLOR_GRAY,
	RECOVERY_COLOR_PINK,
	RECOVERY_COLOR_PURPLE,
	RECOVERY_COLOR_CYAN,
	RECOVERY_COLOR_ORANGE,
	RECOVERY_COLOR_YELLOW,
	RECOVERY_COLOR_B_WHITE,
	RECOVERY_COLOR_B_RED,
	RECOVERY_COLOR_B_GREEN,
	RECOVERY_COLOR_B_BLUE,
};

enum ExtendedColors {
	EXTENDED_COLOR_DISABLED,
	/** PSP 2000 Extended Color */
	EXTENDED_COLOR_02G,
	/** PSP 3000 Extended Color */
	EXTENDED_COLOR_03G,
};

enum HidePicsOpt {
	PICS_OPT_DISABLED,
	PICS_OPT_BOTH,
	PICS_OPT_PIC0_ONLY,
	PICS_OPT_PIC1_ONLY,
};

enum IsoUmdMode {
	ISO_MODE_INFERNO,
	ISO_MODE_MARCH33,
	ISO_MODE_NP9660,
};

enum OskTypes {
	OSK_TYPE_VITA,
	OSK_TYPE_PSP,
};

enum GeTypes {
	GE_TYPE_1,
	GE_TYPE_2,
};

enum MeTypes {
	ME_TYPE_1,
	ME_TYPE_2,
};

enum FakeMaxFreeMemOpt {
	FAKE_MAX_FREE_MEM_AUTO,
	FAKE_MAX_FREE_MEM_16MB,
	FAKE_MAX_FREE_MEM_24MB,
	FAKE_MAX_FREE_MEM_32MB,
	FAKE_MAX_FREE_MEM_40MB,
	FAKE_MAX_FREE_MEM_48MB,
	FAKE_MAX_FREE_MEM_52MB,
	FAKE_MAX_FREE_MEM_60MB,
	FAKE_MAX_FREE_MEM_64MB,
};

/**
 * Configuration for the `Pentazemin` module.
 */
typedef struct PentazeminConfig {
	/** The On System Keyboard module to load. One of `OskTypes`. */
	u8 osk_type;
	/** The implementation of the GE module to load. One of `GeTypes`. */
	u8 ge_type;
	/** The implementation of the ME module to load. One of `MeTypes`. */
	u8 me_type;
	/** Whether to patch the stdio to output in the TTY in the VITA-side (used to output debug log). Boolean. */
	u8 patch_stdio;
} PentazeminConfig;


int sctrlSendAdrenalineCmd(int cmd, u32 args);
int sctrlGetUsbState();
int sctrlStartUsb();
int sctrlStopUsb();
int sctrlRebootDevice();
void sctrlPentazeminConfigure(PentazeminConfig* conf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSTEMCTRL_EPI_H_ */