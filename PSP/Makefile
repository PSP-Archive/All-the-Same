
TARGET = allTheSame-test
OBJS = src/audio.o \
       src/mp3player.o \
       src/input.o \
       src/main.o \
       src/video.o \
       ../General/src/random.o \
       ../General/src/game.o \
       ../General/src/inflate.o \
       ../General/src/videoCommon.o \
       ../Posix/network.o \
       ../Posix/system.o \
       ../Posix/file.o \
       gfx/atlasMenu.o \
       sfx/sfxMenuClick.o

INCDIR =
CFLAGS = -G0 -O0 -Wall -I../General/include -DPSP
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspgum -lpspgu -lm -lmad -lpspaudiolib -lpspaudio -lpsppower -lpsprtc

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = All the Same - Test
PSP_EBOOT_ICON = gfx/icon.png
PSP_EBOOT_PIC1 = gfx/background.png

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

gfx/atlasMenu.o: gfx/atlasMenu.dtt
	bin2o -i gfx/atlasMenu.dtt gfx/atlasMenu.o atlasMenu

sfx/sfxMenuClick.o: sfx/sfxMenuClick.raw
	bin2o -i sfx/sfxMenuClick.raw sfx/sfxMenuClick.o sfxMenuClick
