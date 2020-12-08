CC=gcc
CFLAGS=-lmem -L.
all: mem mymain
mem: mem.c
	$(CC) -c -fpic mem.c -Wall -Werror
	$(CC) -shared -o libmem.so mem.o

mymain: mymain.c
	$(CC) mymain.c -o myprogram $(CFLAGS) -Wall
clean:
	rm *.o
