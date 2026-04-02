#ifndef __SCTRLLIBRARY_SE_H__
#define __SCTRLLIBRARY_SE_H__

#include <psptypes.h>

/**
 * These functions are only available in SE-C and later,
 * and they are not in HEN
*/
enum fakeregion {
    FAKE_REGION_DISABLED = 0,
    FAKE_REGION_JAPAN = 1,
    FAKE_REGION_AMERICA = 2,
    FAKE_REGION_EUROPE = 3,
    FAKE_REGION_KOREA = 4,
    FAKE_REGION_UNITED_KINGDOM = 5,
    FAKE_REGION_LATIN_AMERICA = 6,
    FAKE_REGION_AUSTRALIA = 7,
    FAKE_REGION_HONGKONG = 8,
    FAKE_REGION_TAIWAN = 9,
    FAKE_REGION_RUSSIA = 10,
    FAKE_REGION_CHINA = 11,
    FAKE_REGION_DEBUG_TYPE_I = 12,
    FAKE_REGION_DEBUG_TYPE_II = 13,
};

enum SEUmdModes
{
    MODE_UMD = 0,
    MODE_OE_LEGACY, // not available anymore, will default to inferno
    MODE_MARCH33,
    MODE_NP9660, // (Galaxy) if not available on ARK-4, will default to inferno for iso/cso or np9660 for PBP
    MODE_INFERNO,
    MODE_ME,
    MODE_VSHUMD,
    MODE_UPDATERUMD,
    MODE_RECOVERY,
};

enum swap_xo
{
    XO_CURRENT_O_PRIMARY = 0,
    XO_CURRENT_X_PRIMARY = 1
};

enum convert_battery
{
    NORMAL_TO_PANDORA    = 0,
    PANDORA_TO_NORMAL    = 1,
    UNSUPPORTED        	= 2,
};

enum umdregion
{
    // UMD regions
    UMD_REGION_DEFAULT,
    UMD_REGION_AMERICA,
    UMD_REGION_EUROPE,
    UMD_REGION_JAPAN,
};

enum cpubus_clocks
{
	CPU_BUS_CLOCK_DEFAULT,
	CPU_BUS_CLOCK_333,
	CPU_BUS_CLOCK_222,
	CPU_BUS_CLOCK_133,
	CPU_BUS_CLOCK_CUSTOM,
};

// M33's SEConfig (also used by L/ME)
#define SECONFIG_MAGIC_M33 0x47434553
typedef struct SEConfigM33
{
	int magic; /* 0x47434553 *///ABEC
	int hidecorrupt;//abf0
	int	skiplogo;//
	int umdactivatedplaincheck;//hide mac addr
	int gamekernel150;
	int executebootbin;//datac00
	int startupprog;
	int umdmode;
	int useisofsonumdinserted;
	int	vshcpuspeed; //dataAC10
	int	vshbusspeed;
	int	umdisocpuspeed;
	int	umdisobusspeed;
	int fakeregion;//dataAC20
	int freeumdregion;//executeopnssmp
	int	hardresetHB; //data
	int usbdevice;
	int novshmenu;//dataAC30
	int usbcharge;
	int netupdate;
	int hidepng;
	int plugvsh;//
	int pluggame;
	int plugpop;
	int versiontxt;
	int fastms;//
	int uncc;
	int unccc;
	int nuc;
} SEConfigM33;

// PRO's SEConfig
#define SECONFIG_MAGIC_PRO ((((sctrlHENGetVersion() & 0xF)<<16) | sctrlHENGetMinorVersion()) + 0x47434554)
typedef struct SEConfigPRO
{
	int magic; // depends on pro version, calculated at runtime with above formula
	s16 umdmode;
	s16 vshcpuspeed;
	s16 vshbusspeed;
	s16 umdisocpuspeed;
	s16 umdisobusspeed;
	s16 fakeregion;
	s16 usbdevice;
	s16 usbcharge;
	s16 machidden;
	s16 skipgameboot;
	s16 hidepic;
	s16 plugvsh;
	s16 pluggame;
	s16 plugpop;
	s16 flashprot;
	s16 skiplogo;
	s16 useversion;
	s16 useownupdate;
	s16 usenodrm;
	s16 hibblock;
	s16 noanalog;
	s16 oldplugin;
	s16 htmlviewer_custom_save_location;
	s16 hide_cfw_dirs;
	s16 chn_iso;
	s16 msspeed;
	s16 slimcolor;
	s16 iso_cache;
	s16 iso_cache_total_size; // in MB
	s16 iso_cache_num;
	s16 iso_cache_policy;
	s16 usbversion;
	s16 language; /* -1 as autodetect */
	s16 retail_high_memory;
	s16 macspoofer;
} SEConfigPRO;

// Adrenaline's SEConfig
#define SECONFIG_MAGIC_EPI1 0x192EFC3C
#define SECONFIG_MAGIC_EPI2 0x17BEB6AA
typedef struct {
	int magic[2];
	/** 0 - Disabled, 1 - Enabled */
	u8 hide_corrupt;
	/** 0 - Disabled, 1 - Enabled */
	u8	skip_logo;
	/** 0 - Disabled, 1 - Enabled */
	u8 startup_program;
	/** One of `IsoUmdMode` */
	u8 umd_mode;
	/** One of `CpuBusSpeed` */
	u8	vsh_cpu_speed;
	/** One of `CpuBusSpeed` */
	u8	app_cpu_speed;
	/** One of `FakeRegionOptions` */
	u8 fake_region;
	/** 0 - Disabled, 1 - Enabled */
	u8 skip_game_boot_logo;
	/** 0 - Disabled, 1 - Enabled */
	u8 hide_mac_addr;
	/** 0 - Disabled, 1 - Enabled */
	u8 hide_dlcs;
	/** One of `HidePicsOpt` */
	u8 hide_pic0pic1;
	/** One of `ExtendedColors` */
	u8 extended_colors;
	/** 0 - Disabled, 1 - Enabled */
	u8 use_sony_psposk;
	/** 0 - Use, 1 - Do not use */
	u8 no_nodrm_engine;
	/** 0 - Use, 1 - Do not use */
	u8 no_xmb_plugins;
	/** 0 - Use, 1 - Do not use */
	u8 no_game_plugins;
	/** 0 - Use, 1 - Do not use */
	u8 no_pops_plugins;
	/** One of `ForceHighMemory` */
	u8 force_high_memory;
	/** 0 - Disabled, 1 - Enabled */
	u8 execute_boot_bin;
	/** One of `RecoveryColor` */
	u8 recovery_color;
	/** 0 - load xmbctrl, 1 - not load xmbctrl */
	u8 no_xmbctrl;
	/** Inferno cache type. One of `InfernoCacheConf` */
	u8 iso_cache;
	/** Inferno cache partition 2 or 11 (automatic) */
	u8 iso_cache_partition;
	/** Inferno cache size (in KB) for each cache item. One of `IsoCacheSizeConf` */
	u8 iso_cache_size;
	/** Number of inferno cache items. One of `IsoCacheNumberConf` */
	u8 iso_cache_num;
	/** Simulate UMD seek time. Zero - Off, `>0` - seek time factor, i.e. value that will be multiplied on amount of bytes to be read */
	u8 umd_seek;
	/** Simulate UMD seek time. Zero - Off, `>0` - seek time factor, i.e. value that will be multiplied on amount of bytes to be read */
    u8 umd_speed;
	/** Cache `ms0:`. 0 - Use cache, 1 - do not use cache. */
    u8 no_ms_cache;
	/** Use `ge_2.prx` instead of `ge.prx`. 0 - Off, 1 - On. */
	u8 use_ge2;
	/** Use `kermit_me_wrapper_2.prx` instead of `kermit_me_wrapper.prx`. 0- Off, 1 = On. */
	u8 use_me2;
	/** Hide CFW files from games. 0 - Hide, 1 - Do not hide */
	u8 no_hide_cfw_files;
	/** Fakes the maximum amount of free memory for games. One of `FakeMaxFreeMemOpt`  */
	u8 fake_max_free_mem;
	/** Control wether to do tty redirection. 0 - Off, 1 - On. */
	u8 tty_redirection;
} SEConfigEPI;
typedef SEConfigEPI SEConfigADR;
#define IS_ADR_SECONFIG(config) ((((SEConfigEPI*)config)->magic[0] == SECONFIG_MAGIC_EPI1) && (((SEConfigADR*)config)->magic[1] == SECONFIG_MAGIC_EPI2))

// ARK's SEConfig
typedef struct SEConfigARK {
    u32 magic; // ARK_CONFIG_MAGIC
    u16 iso_cache_size_kb; // in KB, automatic
    u16 iso_cache_num; // number of cache slots
    u8 iso_cache_partition;
    u8 iso_cache_type; // 0 = no cache, 1 = LRU, 2 = RR
    u8 umdseek;
    u8 umdspeed;
    u8 cpubus_clock;
    u8 disable_pause;
    u8 hidedlc;
    u8 umdregion;
    u8 vshregion;
    u8 usbdevice;
    u8 usbcharge;
    u8 hidemac;
    u8 noanalog;
    u8 qaflags; // enable QA flags patch
    u8 launcher_mode;
    u8 hidepics;
    u8 usbdevice_rdonly;
    u8 skiplogos;
    u8 noumd;
    u8 hibblock;
    u8 oldplugin;
    u8 msspeed;
    u8 noled; // always false
    u8 wpa2; // patch to use wpa2
    u8 force_high_memory;
    u8 custom_update;
	u16 custom_cpu_clock;
	u16 custom_bus_clock;
} SEConfigARK;

// Forward declaration
typedef union {
    SEConfigM33 m33;
    SEConfigPRO pro;
    SEConfigEPI adr;
    SEConfigARK ark;
} SEConfig;

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * Gets the SE/OE version.
 *
 * @return The SE version.
 *
 * 3.03 OE-A: 0x00000500
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEGetVersion();

/**
 * Gets the SE configuration.
 *
 * Avoid using this function, it may corrupt your program.
 * Use `sctrlSEGetCongiEx` function instead.
 *
 * @param[out] config A pointer to a AdrenalineConfig structure that receives the SE configuration.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEGetConfig(SEConfig *config);

/**
 * Gets the SE configuration
 *
 * @param[out] config A pointer to a `AdrenalineConfig` structure that receives the SE configuration.
 * @param size The size of the structure.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEGetConfigEx(SEConfig *config, int size);

/**
 * Sets the SE configuration.
 *
 * This function can corrupt the configuration in flash, use
 * `sctrlSESetConfigEx` instead.
 *
 * @param[in] config A pointer to a `AdrenalineConfig` structure that has the SE configuration to set.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSESetConfig(SEConfig *config);

/**
 * Sets the SE configuration.
 *
 * @param[in] config A pointer to a `AdrenalineConfig` structure that has the SE configuration to set.
 * @param size The size of the structure.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSESetConfigEx(SEConfig *config, int size);

/**
 * Immediately sets the SE configuration in memory without saving to flash.
 *
 * This function can corrupt the configuration in memory, use `sctrlSEApplyConfigEX` instead.
 *
 * @param[in] config A pointer to a `SEConfig` structure that has the SE configuration to set.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
*/
void sctrlSEApplyConfig(SEConfig *conf);

/**
 * Immediately sets the SE configuration in memory without saving to flash.
 *
 * @param[in] config A pointer to a `SEConfig` structure that has the SE configuration to set.
 *
 * @return `0` on success, and `-1` on error.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
*/
int sctrlSEApplyConfigEX(SEConfig *conf, int size);


/**
 * Get the CFW's internal `SEConfig` structure.
 *
 * @returns A pointer to the internal `SEConfig`.
 *
 * @attention Requires linking to `pspsystemctrl_kernel` stub to be available.
*/
SEConfig* sctrlSEGetConfigInternal();

/**
 * Initiates the emulation of a disc from an ISO9660/CSO file.
 *
 * @param[in] file The path of the ISO/CISO.
 * @param noumd Wether use noumd or not.
 * @param isofs Wether use the custom SE isofs driver or not.
 *
 * @return `0` on success.
 *
 * @note 1. When setting noumd to `1`, isofs should also be set to `1`,
 * otherwise the UMD would be still required.
 *
 * @note 2. The function doesn't check if the file is valid or even if it exists
 * and it may return success on those cases.
 *
 * @note 3. This function is not available in SE for devhook.
 *
 * @example
 *
 * SEConfig config;
 *
 * sctrlSEGetConfig(&config);
 *
 * if (config.usenoumd)
 * {
 *        sctrlSEMountUmdFromFile("ms0:/ISO/mydisc.iso", 1, 1);
 * }
 * else
 * {
 *        sctrlSEMountUmdFromFile("ms0:/ISO/mydisc.iso", 0, config.useisofsonumdinserted);
 * }
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEMountUmdFromFile(char *file, int noumd, int isofs);

/**
 * Umounts an iso.
 *
 * @return `0` on success.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEUmountUmd(void);

/**
 * Forces the umd disc out state.
 *
 * @param out Non-zero for disc out, `0` otherwise.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
void sctrlSESetDiscOut(int out);

/**
 * Sets the disctype.
 *
 * @param type The disctype. One or more of `PspUmdTypes`.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
void sctrlSESetDiscType(int type);

/**
 * Get the disctype.
 *
 * @return The ISO disctype. One or more of `PspUmdTypes`.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
int sctrlSEGetDiscType(void);

/**
 * Gets the current UMD file (kernel only).
 *
 * @return The current UMD file.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
char *sctrlSEGetUmdFile(void);
char *GetUmdFile(void);

/**
 * Sets the current UMD file (kernel only).
 *
 * @param[in] file The UMD file.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
void sctrlSESetUmdFile(const char *file);
void SetUmdFile(const char *file);


/**
 * Sets the boot config file for next reboot
 *
 * @param index - The index identifying the file. One of `SEUmdModes`.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
void sctrlSESetBootConfFileIndex(int index);

/**
 * Gets the boot config file for next reboot.
 *
 * @return The index identifying the file. One of `SEUmdModes`.
 *
 * @attention Requires linking to `pspsystemctrl_user` or `pspsystemctrl_kernel` stubs to be available.
*/
unsigned int sctrlSEGetBootConfFileIndex(void);

/**
 * Helper function to allocate memory on P1 (kernel memory).
 *
 * @param size The amount of bytes to allocate.
 *
 * @return A pointer to allocated buffer, or NULL on error.
 */
void * oe_malloc(unsigned int size);

/**
 * Helper function to allocate aligned memory on P1 (user memory).
 *
 * @param align The alignment.
 * @param size The amount of bytes to allocate.
 *
 * @return A pointer to allocated buffer, or `NULL` on error.
 */
void * oe_memalign(unsigned int align, unsigned int size);

/**
 * Deallocate memory allocated by oe_malloc.
 *
 * @param[in] ptr A pointer to the allocated memory.
 */
void oe_free(void * p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SCTRLLIBRARY_SE_H__ */
