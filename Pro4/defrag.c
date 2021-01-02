#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defrag.h"

FILE* fin;
FILE* fout;

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

void dfs(int p_wr, int p_rd, int dep) {
	if (dep == 0) {
		fseek(fin, 1024 + (data_offset + p_rd) * block_size, SEEK_SET);
		fread(buffer, block_size, 1, fin);
		fseek(fout, 1024 + (data_offset + p_wr) * block_size, SEEK_SET);
		fwrite(buffer, block_size, 1, fout);
		file_size--;
		return;
	}
	for (int i = 0; i < block_size / sizeof(int) && file_size > 0; i++) {
		int p;
		fseek(fin, 1024 + (data_offset + p_rd) * block_size + i * sizeof(int), SEEK_SET);
		fread(&p, sizeof(int), 1, fin);
		cur_data_p++;
		fseek(fout, 1024 + (data_offset + p_wr) * block_size + i * sizeof(int), SEEK_SET);
		fwrite(&cur_data_p, sizeof(int), 1, fout);
		dfs(cur_data_p, p, dep - 1);
	}
}

int main(int argc, char const *argv[])
{
	if (argc < 2) {
		perror("argument error!");
		return -1;
	}
	fin = fopen(argv[1], "r");
	if (!fin) {
		perror("open input error!");
		return -1;
	}
	fout = fopen(strcat((char*)argv[1], "-defrag"), "w+");
	if (!fout) {
		fclose(fin);
		perror("open output error!");
		return -1;
	}
	// write boot
	boot_c = (char*)malloc(512);
	fseek(fin, 0, SEEK_SET);
	fread(boot_c, 512, 1, fin);
	fseek(fout, 0, SEEK_SET);
	fwrite(boot_c, 512, 1, fout);

	supblock = (superblock_t *)malloc(512);
	fread(supblock, 512, 1, fin);
	block_size = supblock->size;
	inode_offset = supblock->inode_offset;
	data_offset = supblock->data_offset;
	swap_offset = supblock->swap_offset;

	num_inode = (data_offset * block_size - inode_offset * block_size) / sizeof(inode_t); 

	inode = (inode_t *)malloc(sizeof(inode_t));
	buffer = (char*)malloc(block_size);
	cur_data_p = -1;
	for (int i = 0; i < num_inode; i++) {
		fseek(fin, 1024 + inode_offset*block_size + i * sizeof(inode_t), SEEK_SET);
		fread(inode, sizeof(inode_t), 1, fin);
		if (inode->nlink > 0) {
			file_size = (inode->size + block_size - 1) / block_size;
			int nwp;
			for (int j = 0; j < N_DBLOCKS && file_size > 0; j++) {
				nwp = ++cur_data_p;
				dfs(nwp, inode->dblocks[j], 0);
				inode->dblocks[j] = nwp;
			}
			for (int j = 0; j < N_IBLOCKS && file_size > 0; j++) {
				nwp = ++cur_data_p;
				dfs(nwp, inode->dblocks[j], 1);
				inode->iblocks[j] = nwp;
			}
			if (file_size > 0) {
				nwp = ++cur_data_p;
				dfs(nwp, inode->i2block, 2);
				inode->i2block = nwp;
			}
			if (file_size > 0) {
				nwp = ++cur_data_p;
				dfs(nwp, inode->i2block, 3);
				inode->i3block = nwp;
			}
			fseek(fout, 1024 + inode_offset*block_size + i * sizeof(inode_t), SEEK_SET);
			fwrite(inode, sizeof(inode_t), 1, fout);
		}
	}
	supblock->free_iblock = ++cur_data_p;
	fseek(fout, 512, SEEK_SET);
	fwrite(supblock, 512, 1, fout);
	for (int i = cur_data_p; i < swap_offset; i++) {
		fseek(fout, 1024 + i * block_size, SEEK_SET);
		int j = i + 1;
		if (i == swap_offset - 1)j = -1;
		fwrite(&j, sizeof(int), 1, fout);
	}
	fclose(fin);
	fclose(fout);
	free(boot_c);
	free(supblock);
	free(buffer);
	free(inode);
	return 0;
}