CC=gcc
test_counter: xchg futex lock counter list hash counter_test
test_list: xchg futex lock counter list hash list_test
test_hash: xchg futex lock counter list hash hash_test
xchg: xchg.c 
	$(CC) -c -fpic xchg.c -Wall -Werror -std=gnu99
	$(CC) -shared -o libxchg.so xchg.o
futex: sys_futex.c
	$(CC) -c -fpic sys_futex.c -Wall
	$(CC) -shared -o libsys_futex.so sys_futex.o
lock: lock.c
	$(CC) -c -fpic lock.c -lxchg -lsys_futex -L. -Wall -pthread
	$(CC) -shared -o liblock.so lock.o 
counter: counter.c
	$(CC) -c -fpic counter.c -llock -L. -Wall -pthread
	$(CC) -shared -o libcounter.so counter.o
list: list.c
	$(CC) -c -fpic list.c -llock -L. -Wall -pthread
	$(CC) -shared -o liblist.so list.o
hash: hash.c
	$(CC) -c -fpic hash.c -llist -llock -L. -Wall -pthread
	$(CC) -shared -o libhash.so hash.o
counter_test: counter_test.c
	$(CC) counter_test.c -o counter_test -lhash -llist -lcounter -llock -lsys_futex -lxchg -L. -Wall -pthread
list_test: list_test.c
	$(CC) list_test.c -o list_test -lhash -llist -lcounter -llock -lsys_futex -lxchg -L. -Wall -pthread
hash_test: hash_test.c
	$(CC) hash_test.c -o hash_test -lhash -llist -lcounter -llock -lsys_futex -lxchg -L. -Wall -pthread
clean:
	rm *.o
