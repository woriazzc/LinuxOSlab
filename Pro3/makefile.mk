CC=gcc
CFLAGS=-lxchg -L.

all: mem mymain
mem: xchg.c
	$(CC) -c -fpic xchg.c -Wall -Werror
	$(CC) -shared -o libxchg.so xchg.o

mymain: lock.c
	$(CC) lock.c -o myprogram $(CFLAGS) -Wall
clean:
	rm *.o
