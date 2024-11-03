/*
This file is a straight modification of the one provided as an example by
professor Francesco Quaglia <francesco.quaglia@uniroma2.it> during the AOS lectures
source can be found at https://francescoquaglia.github.io/TEACHING/AOS/AA-2023-2024/SOFTWARE/VIRTUAL-FILE-SYSTEM.tar
or more recent editions.
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "logfs.h"

/*
	This makefs will write the following information onto the disk
	- BLOCK 0, superblock;
	- BLOCK 1, inode of the unique file (the inode for root is volatile);
	- BLOCK 2, ..., datablocks of the unique file 
*/

int main(int argc, char *argv[])
{
	int fd, nbytes;
	ssize_t ret;
	struct onefilefs_sb_info sb;
	struct onefilefs_inode root_inode;
	struct onefilefs_inode file_inode;
	struct onefilefs_dir_record record;
	char *block_padding;

	if (argc != 3) {
		printf("Usage: mkfs-singlefilefs node-blocks <device>\n");
		return -1;
	}

	int blocks = strtol(argv[1], NULL, 10) ;

	fd = open(argv[2], O_RDWR);
	if (fd == -1) {
		perror("Error opening the device");
		return -1;
	}

	//pack the superblock
	sb.version = 1;//file system version
	sb.magic = MAGIC;
	sb.block_size = DEFAULT_BLOCK_SIZE;

	ret = write(fd, (char *)&sb, sizeof(sb));

	if (ret != DEFAULT_BLOCK_SIZE) {
		printf("Bytes written [%d] are not equal to the default block size.\n", (int)ret);
		close(fd);
		return ret;
	}

	printf("Super block written succesfully\n");

	// write file inode
	file_inode.mode = S_IFREG;
	file_inode.inode_no = SINGLEFILEFS_FILE_INODE_NUMBER;
	file_inode.file_size = blocks * DEFAULT_BLOCK_SIZE;
	file_inode.circular_buffer_start = file_inode.circular_buffer_end = 0 ;
	printf("File size is %ld\n",file_inode.file_size);
	fflush(stdout);
	ret = write(fd, (char *)&file_inode, sizeof(file_inode));

	if (ret != sizeof(root_inode)) {
		printf("The file inode was not written properly.\n");
		close(fd);
		return -1;
	}

	printf("File inode written succesfully.\n");
	
	//padding for block 1
	nbytes = DEFAULT_BLOCK_SIZE - sizeof(file_inode);
	block_padding = malloc(nbytes);

	ret = write(fd, block_padding, nbytes);

	if (ret != nbytes) {
		printf("The padding bytes are not written properly. Retry your mkfs\n");
		close(fd);
		return -1;
	}
	printf("Padding in the inode block written sucessfully.\n");

	close(fd);

	return 0;
}
