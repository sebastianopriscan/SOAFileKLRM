/*
This file is a straight modification of the one provided as an example by
professor Francesco Quaglia <francesco.quaglia@uniroma2.it> during the AOS lectures
source can be found at https://francescoquaglia.github.io/TEACHING/AOS/AA-2023-2024/SOFTWARE/VIRTUAL-FILE-SYSTEM.tar
or more recent editions.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/timekeeping.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "include/logfs/logfs.h"

#define MODNAME "SOAFileKLRM"

long long circular_buffer_start, circular_buffer_end ;
rwlock_t log_file_lock;
uint64_t file_size ;

ssize_t onefilefs_read(struct file * filp, char __user * buf, size_t len, loff_t * off) {

    read_lock(&log_file_lock) ;

    struct buffer_head *bh = NULL;
    struct inode * the_inode = filp->f_inode;
    uint64_t file_size = the_inode->i_size;
    int ret;
    loff_t offset, circular_pos;
    int block_to_read;//index of the block to be read from device

    printk("%s: read operation called with len %ld - and offset %lld (the current file size is %lld)",MOD_NAME, len, *off, file_size);

    //this operation is not synchronized 
    //*off can be changed concurrently 
    //add synchronization if you need it for any reason

    //check that *off is within boundaries
    if (*off < circular_buffer_start) {
        *off = circular_buffer_start ;
    }
    if (*off >= circular_buffer_end) {
        read_unlock(&log_file_lock) ;
        return 0;
    }
    else if (*off + len > circular_buffer_end)
        len = circular_buffer_end - *off;
    

    //determine the block level offset for the operation
    circular_pos = *off % file_size ;
    if ((*off + len) % file_size < circular_pos) {
        len = file_size - circular_pos ;
    }
    offset = circular_pos % DEFAULT_BLOCK_SIZE; 
    //just read stuff in a single block - residuals will be managed at the applicatin level
    if (offset + len > DEFAULT_BLOCK_SIZE)
        len = DEFAULT_BLOCK_SIZE - offset;

    //compute the actual index of the the block to be read from device
    block_to_read = circular_pos / DEFAULT_BLOCK_SIZE + 2; //the value 2 accounts for superblock and file-inode on device
    
    printk("%s: read operation must access block %d of the device",MOD_NAME, block_to_read);

    bh = (struct buffer_head *)sb_bread(filp->f_path.dentry->d_inode->i_sb, block_to_read);
    if(!bh){
    read_unlock(&log_file_lock) ;
	return -EIO;
    }
    ret = copy_to_user(buf,bh->b_data + offset, len);
    *off += (len - ret);
    brelse(bh);

    read_unlock(&log_file_lock) ;
    return len - ret;

}

void internal_logfilefs_write(char *payload) {

    long long orig_end = circular_buffer_end ;
    size_t payloadlen = strlen(payload) ;
    size_t copied = 0 ;
    struct buffer_head *bh ;
    long long selected_sector ;

    write_lock(&log_file_lock) ;
    if (!mounted) {
        printk("SOAFileKLRM : logging fs not mounted") ;
        write_unlock(&log_file_lock) ;
        return ;
    }

    circular_buffer_end += payloadlen ;
    if (circular_buffer_end < file_size) {

        if (circular_buffer_end + payloadlen > file_size) {
            circular_buffer_start += payloadlen - file_size ;
        } 
    } else {
        circular_buffer_start += payloadlen ;
    }

    do {
        long long circular_buffer_pos, pos_index ;
        ssize_t delta = payloadlen - copied ;

        circular_buffer_pos = (orig_end + copied) % file_size ;
        pos_index = circular_buffer_pos % DEFAULT_BLOCK_SIZE ;
        if (pos_index + delta > DEFAULT_BLOCK_SIZE) {
            delta = DEFAULT_BLOCK_SIZE - pos_index ;
        }
        selected_sector = (circular_buffer_pos / DEFAULT_BLOCK_SIZE) +2 ;
        bh = (struct buffer_head *)sb_bread(singleton_sb, selected_sector);
        if (!bh) {
            write_unlock(&log_file_lock) ;
            printk("SOAFileKLRM : sb_bread error, logging aborted") ;
            return ;
        }
        memcpy(bh->b_data + pos_index, payload + copied, delta) ;
        copied += delta ;
        brelse(bh) ;
    } while(copied != payloadlen) ;

    write_unlock(&log_file_lock) ;
}

struct dentry *onefilefs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flags) {

    struct onefilefs_inode *FS_specific_inode;
    struct super_block *sb = parent_inode->i_sb;
    struct buffer_head *bh = NULL;
    struct inode *the_inode = NULL;

    printk("%s: running the lookup inode-function for name %s",MOD_NAME,child_dentry->d_name.name);

    if(!strcmp(child_dentry->d_name.name, UNIQUE_FILE_NAME)){

	
	//get a locked inode from the cache 
        the_inode = iget_locked(sb, 1);
        if (!the_inode)
       		 return ERR_PTR(-ENOMEM);

	//already cached inode - simply return successfully
	if(!(the_inode->i_state & I_NEW)){
		return child_dentry;
	}


	//this work is done if the inode was not already cached
	inode_init_owner(the_inode, NULL, S_IFREG );
	the_inode->i_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH;
        the_inode->i_fop = &onefilefs_file_operations;
	the_inode->i_op = &onefilefs_inode_ops;

	//just one link for this file
	set_nlink(the_inode,1);

	//now we retrieve the file size via the FS specific inode, putting it into the generic inode
    	bh = (struct buffer_head *)sb_bread(sb, SINGLEFILEFS_INODES_BLOCK_NUMBER );
    	if(!bh){
		iput(the_inode);
		return ERR_PTR(-EIO);
    	}
	FS_specific_inode = (struct onefilefs_inode*)bh->b_data;
	the_inode->i_size = FS_specific_inode->file_size;
        brelse(bh);

        d_add(child_dentry, the_inode);
	dget(child_dentry);

	//unlock the inode to make it usable 
    	unlock_new_inode(the_inode);

	return child_dentry;
    }

    return NULL;

}

//look up goes in the inode operations
const struct inode_operations onefilefs_inode_ops = {
    .lookup = onefilefs_lookup,
};

const struct file_operations onefilefs_file_operations = {
    .owner = THIS_MODULE,
    .read = onefilefs_read,
    //.write = onefilefs_write //please implement this function to complete the exercise
};
