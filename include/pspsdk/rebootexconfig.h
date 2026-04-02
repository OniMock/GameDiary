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

#ifndef _REBOOTCONFIG_H_
#define _REBOOTCONFIG_H_

#include <pspsdk.h>

#define REBOOTEX_MAX_SIZE 0x5000

// Reboot Buffer Configuration Address
#define REBOOTEX_CONFIG 0x88FB0000

// PROCFW/ARK Reboot Buffer ISO Path (so we don't lose that information)
#define REBOOTEX_CONFIG_ISO_PATH_MAXSIZE 0x100

// L/ME
#define REBOOTEX_FILELEN_MAX_LME 0x50

/**
    Originally ARK used a similar rebootex config as PRO with the same magic number.
    Yet it wasn't fully binary compatible with PRO's rebootex config.
    Now we are using a different magic number to identify ARK's rebootconfig vs PRO's.
    This allows ARK SystemControl to work with PRO's reboot buffer configuration.

    If ARK can't detect rebootex configuration, several CFW functions that work with it will be disabled
        (such as being able to change ISO driver, ISO path, reboot module and such).
*/

// ARK Rebootex config
typedef struct RebootexConfigARK {
    unsigned int magic; // ARK magic
    unsigned int reboot_buffer_size; // size of this struct (redundancy)
    unsigned char iso_mode; // Inferno, NP9660
    unsigned char iso_disc_type; // mounted ISO disc type (GAME/VIDEO/AUDIO)
    char iso_path[REBOOTEX_CONFIG_ISO_PATH_MAXSIZE]; // inferno ISO path
    struct { // runtime module, gets injected into boot sequence during reboot
        char *before;
        void *buffer;
        u32 size;
        u32 flags;
    } rtm_mod;
    char game_id[10]; // GameID of currently running game
    unsigned int boot_from_fw_version; // for TimeMachine
    int fake_apitype; // for pspemu ef0 support
    struct { // information about last played
        int apitype;
        char game_id[10];
        char path[REBOOTEX_CONFIG_ISO_PATH_MAXSIZE];
    } last_played;
} RebootexConfigARK;

typedef struct RebootexConfigEPI {
    int bootfileindex;
    u8 iso_disc_type;

    char *module_after;
    void *buf;
    int size;
    int flags;

    u32 ram2;
    u32 ram11;

    char umdfilename[256];
    char title_id[10];
} RebootexConfigEPI;
typedef RebootexConfigEPI RebootexConfigADR;

// L/ME
typedef struct RebootexConfigLME {
    char    file[REBOOTEX_FILELEN_MAX_LME];//0
    u32     config[0x70/4];//0x50
    int     reboot_index;//0xc0
    int     mem2;
    int     mem8;
    int     k150_flag;
    void*   on_reboot_after;
    void*   on_reboot_buf;
    int     on_reboot_size;
    int     on_reboot_flag;
} RebootexConfigLME;

typedef struct RebootexConfigPRO {
    u32 magic;
    u32 rebootex_size;
    u32 p2_size;
    u32 p9_size;
    char *insert_module_before;
    void *insert_module_binary;
    u32 insert_module_size;
    u32 insert_module_flags;
    u32 psp_fw_version;
    u8 psp_model;
    u8 iso_mode;
    u8 recovery_mode;
    u8 ofw_mode;
    u8 iso_disc_type;
} RebootexConfigPRO;

typedef union {
    RebootexConfigLME lme;
    RebootexConfigPRO pro;
    RebootexConfigARK ark;
    RebootexConfigEPI adr;
} RebootexConfig;


#endif

