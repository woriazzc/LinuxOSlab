#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defrag.h"

FILE* f;

int block_size;
char* boot_c;

superblock_t* supblock;
inode_t* inode;
int inode_offset;
int data_offset;
int swap_offset;
int num_inode;

int cur_data_p;
int file_size;
char* buffer;

void dfsp(int p, int dep) {
	printf("%d ", p);
	if (dep == 0) {
		file_size--;
		return;
	}
	for (int i = 0; i < block_size / sizeof(int) && file_size > 0; i++) {
		fseek(f, 1024 + (data_offset + p) * block_size + i * sizeof(int), SEEK_SET);
		int t;
		fread(&t, sizeof(int), 1, f);
		dfsp(t, dep - 1);
	}
}
void print(inode_t* inode) {
	if(inode->nlink == 0)return;
	file_size = (inode->size + block_size - 1) / block_size;
	//printf("%d\n", file_size);
	for (int j = 0; j < N_DBLOCKS && file_size > 0; j++) {
		dfsp(inode->dblocks[j], 0);
	}
	for (int j = 0; j < N_IBLOCKS && file_size > 0; j++) {
		dfsp(inode->iblocks[j], 1);
	}
	if (file_size > 0) {
		dfsp(inode->i2block, 2);
	}
	if (file_size > 0) {
		dfsp(inode->i3block, 3);
	}
	//printf("%d\n", inode->size);
	puts("\n");
}
int main(int argc, char const *argv[])
{
	f = fopen(argv[1], "r");
	supblock = (superblock_t *)malloc(512);
	boot_c = (char*)malloc(512);
	fseek(f, 0, SEEK_SET);
	fread(boot_c, 512, 1, f);
	fread(supblock, 512, 1, f);
	block_size = supblock->size;
	inode_offset = supblock->inode_offset;
	data_offset = supblock->data_offset;
	swap_offset = supblock->swap_offset;

	buffer = (char*)malloc(block_size);
	num_inode = (data_offset * block_size - inode_offset * block_size) / sizeof(inode_t); 

	inode = (inode_t *)malloc(sizeof(inode_t));
	cur_data_p = -1;

	printf("block_size = %d\n", supblock->size);
	printf("inode_offset = %d\n", supblock->inode_offset);
	printf("data_offset = %d\n", supblock->data_offset);
	printf("swap_offset = %d\n", supblock->swap_offset);
	printf("free_inode = %d\n", supblock->free_inode);
	printf("free_iblock = %d\n", supblock->free_iblock);

	int p = supblock->free_iblock;
	while(p != -1){
		//printf("free_iblock: %d\n", p);
		fseek(f, 1024 + p*block_size, SEEK_SET);
		fread(&p, sizeof(int), 1, f);
	}
	
	for (int i = 0; i < num_inode; i++) {
		fseek(f, 1024 + inode_offset*block_size + i * sizeof(inode_t), SEEK_SET);
		fread(inode, sizeof(inode_t), 1, f);
		printf("inode: %d\n", i);
		print(inode);
	}
	fclose(f);
	free(boot_c);
	free(supblock);
	free(buffer);
	free(inode);
	return 0;
}