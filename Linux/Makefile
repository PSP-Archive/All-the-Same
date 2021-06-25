
CC = gcc
LD = gcc

CFLAGS = -Wall -c -g -std=c99 -I../General/include -DLINUX -DSDL -I/usr/local/include/AL
GFLAGS = $(CFLAGS) `sdl-config --cflags`
LDFLAGS =
LDLIBS = `sdl-config --cflags --libs` -L/usr/X11R6/lib/ -lm -lGLU -lGL -lopenal -lalut -lvorbis -lvorbisfile -logg

CFILES = ../SDL/main.c \
         ../SDL/input.c \
         ../OpenGL/video.c \
         ../OpenAL/audio.c \
         ../General/src/game.c \
         ../General/src/random.c \
         ../General/src/inflate.c \
         ../General/src/videoCommon.c \
         ../Posix/file.c \
         ../Posix/network.c \
         ../Posix/system.c
HFILES = ../General/include/main.h \
         ../General/include/input.h \
         ../General/include/video.h \
         ../General/include/audio.h \
         ../General/include/game.h \
         ../General/include/random.h \
         ../General/include/inflate.h \
         ../General/include/file.h \
         ../General/include/network.h \
         ../General/include/system.h
OFILES = main.o input.o video.o audio.o game.o random.o inflate.o videoCommon.o file.o network.o system.o
EXECUT = allTheSame-test


all: $(OFILES) Makefile
	$(LD) $(LDFLAGS) $(OFILES) -o $(EXECUT) $(LDLIBS)


main.o: ../SDL/main.c
	$(CC) $(GFLAGS) ../SDL/main.c

input.o: ../SDL/input.c
	$(CC) $(GFLAGS) ../SDL/input.c

video.o: ../OpenGL/video.c
	$(CC) $(GFLAGS) ../OpenGL/video.c

audio.o: ../OpenAL/audio.c
	$(CC) $(GFLAGS) ../OpenAL/audio.c

game.o: ../General/src/game.c
	$(CC) $(GFLAGS) ../General/src/game.c

random.o: ../General/src/random.c
	$(CC) $(GFLAGS) ../General/src/random.c

inflate.o: ../General/src/inflate.c
	$(CC) $(GFLAGS) ../General/src/inflate.c

videoCommon.o: ../General/src/videoCommon.c
	$(CC) $(GFLAGS) ../General/src/videoCommon.c

file.o: ../Posix/file.c
	$(CC) $(GFLAGS) ../Posix/file.c

network.o: ../Posix/network.c
	$(CC) $(GFLAGS) ../Posix/network.c

system.o: ../Posix/system.c
	$(CC) $(GFLAGS) ../Posix/system.c


$(OFILES): $(HFILES)


clean:
	rm -f $(OFILES) core *~ $(EXECUT) gmon.out

nice:
	rm -f *~
