#ifndef _MODELS_H_
#define _MODELS_H_

#include <pspkernel.h>

#define CAT_PSP 0
#define CAT_PS1 1
#define CAT_HOMEBREW 2
#define CAT_VSH 3
#define CAT_UNKNOWN 4

typedef struct {
  char game_id[16];
  char game_name[64];
  char apitype_str[8];
  u8 category;
} GameMetadata;

#endif // _MODELS_H_
