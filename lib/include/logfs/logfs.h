#ifndef _LOGFS_H
#define _LOGFS_H

#include <linux/types.h>
#include <linux/fs.h>


#define MOD_NAME "SINGLE FILE FS"

#define MAGIC 0x69420694
#define DEFAULT_BLOCK_SIZE 4096
#define SB_BLOCK_NUMBER 0
#define DEFAULT_FILE_INODE_BLOCK 1

#define FILENAME_MAXLEN 255

#define SINGLEFILEFS_ROOT_INODE_NUMBER 10
#define SINGLEFILEFS_FILE_INODE_NUMBER 1

#define SINGLEFILEFS_INODES_BLOCK_NUMBER 1

#define UNIQUE_FILE_NAME "SOAFileKLRM.log"

//inode definition
struct onefilefs_inode {
	mode_t mode;//not exploited
	uint64_t inode_no;
	uint64_t data_block_number;//not exploited

	long long circular_buffer_start;
	long long circular_buffer_end;

	union {
		uint64_t file_size;
		uint64_t dir_children_count;
	};
};

//dir definition (how the dir datablock is organized)
struct onefilefs_dir_record {
	char filename[FILENAME_MAXLEN];
	uint64_t inode_no;
};


//superblock definition
struct onefilefs_sb_info {
	uint64_t version;
	uint64_t magic;
	uint64_t block_size;
	uint64_t inodes_count;//not exploited
	uint64_t free_blocks;//not exploited

	//padding to fit into a single block
	char padding[ (4 * 1024) - (5 * sizeof(uint64_t))];
};

// file.c
extern const struct inode_operations onefilefs_inode_ops;
extern const struct file_operations onefilefs_file_operations; 
extern struct mutex log_file_mutex;
extern uint64_t file_size ;
extern long long circular_buffer_start;
extern long long circular_buffer_end;

// dir.c
extern const struct file_operations onefilefs_dir_operations;

// logfilefs_src.c
extern struct super_block *singleton_sb ;
extern volatile unsigned int mounted ;
extern spinlock_t sb_lock ;

int singlefilefs_init(void) ;

void singlefilefs_exit(void) ;

#endif
