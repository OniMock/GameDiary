TARGET = GameDiary

OBJS = src/main.o src/apitype.o src/detector.o src/storage.o src/tracker.o

INCDIR = include
CFLAGS = -O2 -Os -G0 -Wall -Wextra -D_PSP_FW_VERSION=661 \
         -Iinclude \
         -Iinclude/ark \
         -I/usr/local/pspdev/psp/include \
         -I/usr/local/pspdev/psp/sdk/include
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1
PSP_FW_VERSION = 661

LIBS = -lpsppower -lpsprtc -lpspctrl_driver -lpspdisplay_driver -lpspsystemctrl_kernel -lpspkubridge -lgcc
LDFLAGS = -Llib

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
