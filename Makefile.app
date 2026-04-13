TARGET = GameDiaryApp

# -----------------------------------------------------------------------
# App objects — user-mode
# -----------------------------------------------------------------------
APP_OBJS = \
  src/app/main.o

# -----------------------------------------------------------------------
# Common objects — shared between plugin and app
# -----------------------------------------------------------------------
COMMON_OBJS = \
  src/common/storage.o \
  src/common/sfo_parser.o \
  src/common/utils.o

OBJS = $(APP_OBJS) $(COMMON_OBJS)

INCDIR = include
CFLAGS = -O2 -Os -G0 -Wall -Wextra -D_PSP_FW_VERSION=661 -DGDIARY_APP \
         -Iinclude \
         -Ilib \
         -I/usr/local/pspdev/psp/include \
         -I/usr/local/pspdev/psp/sdk/include
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

# Standard user-mode EBOOT
BUILD_PRX = 0

PSP_FW_VERSION = 661

# Minimal specific user libraries. 
# The PSPSDK build.mak will append standard ones (lpspuser, lpspkernel, etc.) correctly.
LIBS = -lpsppower -lpsprtc -lpspvshbridge
LDFLAGS = -Llib/ark4

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
