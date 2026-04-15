TARGET = GameDiaryApp

# -----------------------------------------------------------------------
# Sources (AUTO — sem lista manual de .o)
# -----------------------------------------------------------------------
APP_SRCS := $(shell find src/app -name "*.c" -o -name "*.cpp")
COMMON_SRCS := $(shell find src/common -name "*.c" -o -name "*.cpp")

SRCS := $(APP_SRCS) $(COMMON_SRCS)

# Build in a separate directory to avoid collisions with the plugin objects
OBJDIR = obj/app
OBJS := $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
OBJS := $(OBJS:.cpp=.o)

# -----------------------------------------------------------------------
# Rules
# -----------------------------------------------------------------------
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# -----------------------------------------------------------------------
# Compiler / flags
# -----------------------------------------------------------------------
INCDIR = include

CFLAGS = -O2 -Os -G0 -Wall -Wextra \
         -D_PSP_FW_VERSION=661 -DGDIARY_APP \
         -Iinclude \
         -Iinclude/app \
         -Ilib

CXXFLAGS = $(CFLAGS)
ASFLAGS  = $(CFLAGS)

# C++ compiler (importante no PSP)
CXX = psp-g++

# -----------------------------------------------------------------------
# Build type (APP ONLY)
# -----------------------------------------------------------------------
BUILD_PRX = 0
PSP_EBOOT_TITLE = Game Diary
PSP_FW_VERSION = 661

# -----------------------------------------------------------------------
# Libraries
# -----------------------------------------------------------------------
LIBS = -lpspgu -lpspgum \
       -lpng -lz -lm \
       -lpsprtc -lpsppower -lpspvshbridge \
       -lstdc++

LIBS += -Llib/ark4

# -----------------------------------------------------------------------
# SDK
# -----------------------------------------------------------------------
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

# -----------------------------------------------------------------------
# Default build
# -----------------------------------------------------------------------
all: EBOOT.PBP
