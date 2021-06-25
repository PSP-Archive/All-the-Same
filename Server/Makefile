
CC = gcc
LD = gcc

CFLAGS = -Wall -c -g -O0 -pedantic -std=c99 -DLINUX
LDFLAGS = 

CFILES = main.c
HFILES = main.h game.h
OFILES = main.o
EXECUT = robotsOfKillServer


all: $(OFILES) Makefile
	$(LD) $(LDFLAGS) $(OFILES) -o $(EXECUT) -lm

main.o: main.c defines.h game.h
	$(CC) $(CFLAGS) main.c


$(OFILES): $(HFILES)


clean:
	rm -f $(OFILES) core *~ $(EXECUT)

nice:
	rm -f *~

install:
	make; cp $(EXECUT) /usr/local/bin/
