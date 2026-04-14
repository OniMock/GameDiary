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
  src/app/render/sdf_font.o \
  src/app/render/font_latin_cyrillic_embed.o \
  src/app/render/font_cjk_embed.o \
  src/app/render/font_symbols_embed.o \
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

# Graphics and system libraries (intrafont removed — replaced by SDF renderer)
LIBS = -lpspgum -lpspgu -lpng -lz -lm -lpsppower -lpsprtc -lpspvshbridge

LDFLAGS = -Llib/ark4

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

# Ensure EBOOT.PBP is created
all: EBOOT.PBP

# -----------------------------------------------------------------------
# package: Creates a deployable folder ready to copy to the PSP memory
# stick at ms0:/PSP/GAME/GameDiaryApp/
#
# Usage:   make -f Makefile.app package
# Deploy:  Copy pkg/GameDiaryApp/ to ms0:/PSP/GAME/GameDiaryApp/
# -----------------------------------------------------------------------
PKG_DIR = pkg/GameDiaryApp

package: EBOOT.PBP
	mkdir -p $(PKG_DIR)/assets/fonts
	cp EBOOT.PBP $(PKG_DIR)/
	cp assets/fonts/font_latin_cyrillic.png $(PKG_DIR)/assets/fonts/
	cp assets/fonts/font_latin_cyrillic.bin $(PKG_DIR)/assets/fonts/
	cp assets/fonts/font_cjk.png            $(PKG_DIR)/assets/fonts/
	cp assets/fonts/font_cjk.bin            $(PKG_DIR)/assets/fonts/
	cp assets/fonts/font_symbols.png        $(PKG_DIR)/assets/fonts/
	cp assets/fonts/font_symbols.bin        $(PKG_DIR)/assets/fonts/
	@echo ""
	@echo "Package ready: $(PKG_DIR)/"
	@echo "Copy to PSP: ms0:/PSP/GAME/GameDiaryApp/"
