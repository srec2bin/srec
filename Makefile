# Makefile

CC=mingw32-gcc

CFLAGS=-c -Wall
LDFLAGS=-fno-exceptions -s

all: bin2srec.exe srec2bin.exe

%.exe:%.o
	$(CC) $(LDFLAGS) $< -o $@

%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	-$(RM) *.o
	-$(RM) *.exe