#ifndef _MODELS_H_
#define _MODELS_H_

#include <pspkernel.h>

typedef struct {
  char game_id[16];
  char game_name[64];
  char apitype_str[8];
  u8 category;
} GameMetadata;

#endif // _MODELS_H_
