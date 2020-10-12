#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"
#include <sys/types.h>
#include <sys/stat.h>

void
usage(const char *prog) 
{
    fprintf(stderr, "usage: %s <inputFile> <outputFile>\n", prog);
    exit(1);
}

int cmp(const void* a, const void* b){
	return ((rec_t*)a)->key - ((rec_t*)b)->key;
}

int
main(int argc, char const *argv[])
{
	assert(sizeof(rec_t) == 100);

	char* inFile = "/no/such/file";
	char* outFile = "/no/such/file";

	if(argc != 3){
		usage(argv[0]);
	}

	inFile = strdup(argv[1]);
	outFile = strdup(argv[2]);

	int fd = open(inFile, O_RDONLY);
	if(fd < 0){
		perror("open input");
		exit(1);
	}

	struct stat buf;
	stat(inFile, &buf);
	int siz = buf.st_size;
	int n = siz / (sizeof(rec_t));

	rec_t* r;
	r = (rec_t*)malloc(n * sizeof(rec_t));

	for(int i = 0; i < n; i++){
		read(fd, &r[i].key, sizeof(unsigned int));
		for(int j = 0; j < NUMRECS; j++){
			read(fd, &r[i].record[j], sizeof(unsigned int));
		}
	}

	qsort(r, n, sizeof(rec_t), cmp);

	close(fd);

	fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if(fd < 0){
		perror("open output");
		exit(1);
	}

	int writecnt = write(fd, r, siz);
	if(writecnt < siz){
		perror("write");
		exit(1);
	}

	free(r);
	close(fd);
	return 0;
}