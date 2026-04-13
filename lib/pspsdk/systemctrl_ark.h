/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 */

#ifndef _SYSCTRL_ARK_H_
#define _SYSCTRL_ARK_H_

#include <pspsdk.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pointers and sizes
#define ARK_PATH_SIZE 128
#define ARK_CONFIG 0x08800010 // ARK Runtime configuration backup address
#define ARK_CONFIG_MAGIC 0xB00B1E55 // generic magic number
#define VITA_FLASH_ARK 0x8BA00000 // ark's flash ramfs on vita
#define ARK_BIN_MAX_SIZE 0x8000 // max size of ARK4.BIN
#define MAX_FLASH0_SIZE 0x40000 // max size of FLASH0.ARK
#define FAKE_UID 0x0B00B500

// Paths and other global strings
#define VBOOT_PBP "VBOOT.PBP" // default launcher
#define ARK_XMENU "XBOOT.PBP" // PS1 launcher
#define ARK_RECOVERY "RECOVERY.PBP" // recovery app
#define RECOVERY_PRX "RECOVERY.PRX" // Classic Recovery
#define FLASH0_ARK "FLASH0.ARK" // ARK flash0 package
#define CIPL_ARK "CIPL.ARK" // ARK cIPL package
#define VSH_MENU "VSHMENU.PRX" // ARK VSH Menu for XMB
#define XMBCTRL_PRX "XMBCTRL.PRX" // XMB Control
#define IDSREG_PRX "IDSREG.PRX" // idsRegeneration
#define USBDEV_PRX "USBDEV.PRX" // Custom USB Device
#define PS1SPU_PRX "PS1SPU.PRX" // PS1 SPU Plugin
#define PSPFTP_PRX "PSPFTP.PRX" // FTP Server/Client PRX library
#define LIBPNG_PRX "LIBPNG.PRX" // PNG library
#define PSPAV_PRX "PSPAV.PRX" // PSP AT3/PMF/MPS PRX library
#define VLF_PRX "VLF.PRX" // PNG library
#define INTRAFONT_PRX "INTRAFON.PRX" // PNG library
#define UNARCHIVE_PRX "UNARCH.PRX" // Unzip/Unrar PRX library
#define XMBCTRL_PRX_FLASH "flash0:/kd/ark_xmbctrl.prx" // XMB Control flash0 path
#define USBDEV_PRX_FLASH "flash0:/kd/usbdevice.prx" // USBDEV flash0 path
#define IDSREG_PRX_FLASH "flash0:/kd/idsregeneration.prx" // idsRegeneration flash0 path
#define VSH_MENU_FLASH "flash0:/vsh/module/ark_satelite.prx" // VSH Menu flash0 path
#define RECOVERY_PRX_FLASH "flash0:/vsh/module/ark_recovery.prx" // Classic Recovery flash0 path
#define H_FILE "H.BIN" // user exploit binloader
#define K_FILE "K.BIN" // kernel exploit file for Live loaders
#define UPDATER_FILE "UPDATER.TXT" // Update Server URL file
#define ARK_SETTINGS "SETTINGS.TXT" // CFW Settings file
#define MENU_SETTINGS "ARKMENU.BIN" // Settings file for CL and VSH Menu
#define ARK_SETTINGS_FLASH FLASH1_PATH ARK_SETTINGS
#define UPDATER_FILE_FLASH FLASH1_PATH UPDATER_FILE // Update Server URL file flash1 path
#define PLUGINS_FILE "PLUGINS.TXT" // plugins config file
#define SEPLUGINS_MS0 "ms0:/SEPLUGINS/" // plugins folder
#define SEPLUGINS_EF0 "ef0:/SEPLUGINS/" // plugins folder (pspgo internal)
#define PLUGINS_PATH SEPLUGINS_MS0 PLUGINS_FILE
#define PLUGINS_PATH_GO SEPLUGINS_EF0 PLUGINS_FILE
#define PLUGINS_PATH_FLASH FLASH0_PATH PLUGINS_FILE
#define ARK_THEME_FILE "THEME.ARK" // theme file for arkMenu
#define ARK_LANG_FILE "LANG.ARK" // language files
#define ARK_BIN "ARK.BIN" // ARK-2 payload
#define ARK4_BIN "ARK4.BIN" // ARK-4 payload
#define ARKX_BIN "ARKX.BIN" // ARK-X payload
#define LIVE_EXPLOIT_ID "Live" // default loader name
#define CIPL_EXPLOIT_ID "cIPL" // loader name for Custom IPL
#define DC_EXPLOIT_ID "DC" // loader name for Despertar del Cementerio
#define DEFAULT_ARK_FOLDER "ARK_01234"
#define SAVEDATA_MS0 "ms0:/PSP/SAVEDATA/"
#define SAVEDATA_EF0 "ef0:/PSP/SAVEDATA/"
#define DEFAULT_ARK_PATH SAVEDATA_MS0 DEFAULT_ARK_FOLDER "/" // default path for ARK files
#define DEFAULT_ARK_PATH_GO SAVEDATA_EF0 DEFAULT_ARK_FOLDER "/" // default path for ARK files
#define ARK_DC_PATH "ms0:/TM/DCARK"
#define TM_PATH_W L"\\TM\\DCARK\\"

// Syscon mem address used for 1.50 resume support
#define SYSCON_SCRATCHPAD_RESUME_FW_ADDR 0x4

/*
Device identifier. Used as extension of psp_model.
First two bits identify the device (PSP or PS Vita)
Second two bits identify special cases (PSP, Vita, Vita PSX, etc)
Dev Sub
00  00 -> unknown device (attempt to autodetect)
01  00 -> psp
01  01 -> psp toolkit
01  10 -> unused
10  00 -> ps vita
10  01 -> vita adrenaline
10  10 -> vita pops
11  00 -> device mask
*/
typedef enum{
    DEV_UNK = 0b0000,
    PSP_ORIG = 0b0100,
    PSP_TOOL = 0b0101,
    PS_VITA = 0b1000,
    PSV_ADR = 0b1001,
    PSV_POPS = 0b1010,
    DEV_MASK = 0b1100,
}ExecMode;

// These settings should be global and constant during the entire execution of ARK.
// It should not be possible to change these (except for recovery flag).
typedef struct ARKConfig{
    u32 magic;
    char arkpath[ARK_PATH_SIZE-20]; // ARK installation folder, leave enough room to concatenate files
    char exploit_id[12]; // ID of the game exploit, or name of the bootloader
    char launcher[20]; // run ARK in launcher mode if launcher specified
    unsigned char exec_mode; // ARK execution mode (PSP, PS Vita, Vita POPS, etc)
    unsigned char recovery; // run ARK in recovery mode (disables settings, plugins and autoboots RECOVERY.PBP)
} ARKConfig;

// macros for checkig stuff
#define IS_ARK_CONFIG(c) (*((u32*)c) == ARK_CONFIG_MAGIC)
#define IS_PSP(_arkconfigptr_) ((_arkconfigptr_->exec_mode&DEV_MASK)==PSP_ORIG)
#define IS_VITA(_arkconfigptr_) ((_arkconfigptr_->exec_mode&DEV_MASK)==PS_VITA)
#define IS_VITA_ADR(_arkconfigptr_) (_arkconfigptr_->exec_mode==PSV_ADR)
#define IS_VITA_POPS(_arkconfigptr_) (_arkconfigptr_->exec_mode==PSV_POPS)


// menu configuration (launcher/vsh/etc)
typedef struct {
    unsigned char fast_gameboot; // skip pmf/at3 and gameboot animation
    unsigned char language; // default language for the menu
    unsigned char font; // default font (either the ones in flash0 or the custom one in THEME.ARK
    unsigned char syslang; // use System Language
    unsigned char scan_save; // enable or disable scanning savedata
    unsigned char scan_cat; // allow scanning for categorized content in /ISO and /PSP/GAME
    unsigned char show_dlc; // allow scanning for DLC files (PBOOT.PBP)
    unsigned char swap_buttons; // whether to swap Cross and Circle
    unsigned char animation; // the background animation of the menu
    unsigned char main_menu; // default menu opened at startup (game by default)
    unsigned char sort_entries; // sort entries by name
    unsigned char show_recovery; // show recovery menu entry
    unsigned char show_fps; // show menu FPS
    unsigned char text_glow; // text glowing function scale
    unsigned char screensaver; // Screensaver time (or disabled)
    unsigned char redirect_ms0; // redirect ms0 to ef0
    unsigned char vsh_fg_color; // Advanced VSH Menu Forground color
    unsigned char vsh_bg_color; // Advanced VSH Menu Background color
    unsigned char menusize; // Change size of dropdown system menu (triangle)
    unsigned char force_update; // Force update (disable update version check)
    unsigned char battery_percent; // show remaing battery percent next to battery icon
    unsigned char startbtn; // Default (normal start button behaviour or boot last game)
    char last_game[128];    // last played game
    unsigned char vsh_font; // font used by VSH Menu
    char browser_dir[128]; // default directory when using file browser as main app
    unsigned char show_hidden; // show hidden files/folders
    unsigned char browser_icon0; // display ICON0 in File Browser
    unsigned char show_size; // show file size in browser
    unsigned char show_path; // show device in game manager title for GO, whether the game is on ef0 or ms0 
    unsigned char window_mode; // Choose whether to use the Classic VSH Menu Design or the new look
    unsigned char advanced_vsh; // Choose to autoload into advanced vsh menu
    unsigned char avm_hidden[32]; // Hiden items in advanced vsh menu
    unsigned char app_autoboot; //Don't check if user is pressing LT when autobooting last game
} ArkMenuConf;


/*
 * Function to obtain ARK's execution environment configuration.
 */
void* sctrlArkGetConfig(ARKConfig* conf);

/**
 * Exit back to ARK's Custom Launcher.
 * If launcher is unavailable, it will do an exitVSH.
 */
int sctrlArkExitLauncher(void);

#ifdef __cplusplus
}
#endif

#endif

