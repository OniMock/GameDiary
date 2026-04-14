TARGET = GameDiaryApp

# -----------------------------------------------------------------------
# App objects — user-mode
# -----------------------------------------------------------------------
APP_OBJS = \
  src/app/main.o \
  src/app/i18n/i18n.o \
  src/app/i18n/i18n_en.o \
  src/app/i18n/i18n_pt.o \
  src/app/i18n/i18n_es.o \
  src/app/config/config.o \
  src/app/render/renderer.o \
  src/app/render/font.o \
  src/app/render/texture.o \
  src/app/data/data_loader.o \
  src/app/data/stats_calculator.o \
  src/app/ui/screen_manager.o \
  src/app/ui/ui_layout.o \
  src/app/ui/ui_components.o \
  src/app/ui/dashboard.o \
  src/app/ui/stats.o \
  src/app/ui/game_list.o \
  src/app/ui/game_details.o \
  src/app/ui/settings.o

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
         -Iinclude/app \
         -Ilib \
         -I/usr/local/pspdev/psp/include \
         -I/usr/local/pspdev/psp/sdk/include
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

# Standard user-mode EBOOT
BUILD_PRX = 1
PSP_EBOOT_TITLE = Game Diary

PSP_FW_VERSION = 661

# Graphics and system libraries
LIBS = -lintrafont -lpspgum -lpspgu -lpng -lz -lm -lpsppower -lpsprtc -lpspvshbridge

LDFLAGS = -Llib/ark4

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

# Ensure EBOOT.PBP is created
all: EBOOT.PBP
