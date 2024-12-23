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
struct mutex log_file_mutex;
uint64_t file_size ;

ssize_t onefilefs_read(struct file * filp, char __user * buf, size_t len, loff_t * off) {

    struct buffer_head *bh = NULL;
    int ret;
    loff_t offset, circular_pos;
    int block_to_read;//index of the block to be read from device

    mutex_lock(&log_file_mutex) ;

    printk("%s: read operation called with len %ld - and offset %lld (the current file size is %lld)",MOD_NAME, len, *off, file_size);
    printk("%s : circular buffer start is %lld, circular buffer end is %lld", MOD_NAME, circular_buffer_start, circular_buffer_end) ;

    //this operation is not synchronized 
    //*off can be changed concurrently 
    //add synchronization if you need it for any reason

    //check that *off is within boundaries
    if (*off < circular_buffer_start) {
        *off = circular_buffer_start ;
    }
    if (*off >= circular_buffer_end) {
        mutex_unlock(&log_file_mutex) ;
        return 0;
    }
    else if (*off + len > circular_buffer_end)
        len = circular_buffer_end - *off;
    

    //determine the block level offset for the operation
    circular_pos = *off % file_size ;
    printk("%s : position in all buffer is %lld", MOD_NAME, circular_pos) ;
    if ((*off + len) % file_size < circular_pos) {
        len = file_size - circular_pos ;
    }
    offset = circular_pos % DEFAULT_BLOCK_SIZE; 
    //just read stuff in a single block - residuals will be managed at the applicatin level
    if (offset + len > DEFAULT_BLOCK_SIZE)
        len = DEFAULT_BLOCK_SIZE - offset;

    //compute the actual index of the the block to be read from device
    block_to_read = circular_pos / DEFAULT_BLOCK_SIZE + 2; //the value 2 accounts for superblock and file-inode on device
    printk("%s : reading in all block %d", MOD_NAME, block_to_read) ;
    
    printk("%s: read operation must access block %d of the device",MOD_NAME, block_to_read);

    bh = (struct buffer_head *)sb_bread(filp->f_path.dentry->d_inode->i_sb, block_to_read);
    if(!bh){
        mutex_unlock(&log_file_mutex) ;
        return -EIO;
    }
    ret = copy_to_user(buf,bh->b_data + offset, len);
    *off += (len - ret);
    brelse(bh);

    mutex_unlock(&log_file_mutex) ;
    return len - ret;

}

//DEBUG ONLY
ssize_t logfilefs_write(struct file * filp, const char __user *buf, size_t len, loff_t *off ) {

    long long orig_start, orig_end ;
    size_t copied = 0 ;
    struct buffer_head *bh ;
    long long selected_sector ;

    mutex_lock(&log_file_mutex) ;

    orig_start = circular_buffer_start ;
    orig_end = circular_buffer_end ;


    do {
        long long circular_buffer_pos, pos_index ;
        ssize_t delta = len - copied ;
        int copied_result ;

        circular_buffer_pos = (orig_end + copied) % file_size ;
        pos_index = (circular_buffer_pos) % DEFAULT_BLOCK_SIZE ;
        printk("%s: position in all buffer is %lld", MOD_NAME, pos_index) ;
        if (pos_index + delta > DEFAULT_BLOCK_SIZE) {
            delta = DEFAULT_BLOCK_SIZE - pos_index ;
        }
        selected_sector = (circular_buffer_pos / DEFAULT_BLOCK_SIZE) +2 ;
        printk("%s: which resides in %lld block", MOD_NAME, selected_sector) ;
        bh = (struct buffer_head *)sb_bread(singleton_sb, selected_sector);
        if (!bh) {
            printk("SOAFileKLRM : sb_bread error, logging aborted") ;
            break ;
        }
        copied_result = copy_from_user(bh->b_data + pos_index, buf + copied, delta) ;
        if (copied_result != 0) {
            copied += delta - copied_result ;
            brelse(bh) ;
            printk("SOAFileKLRM : copy_from_user_error") ;
            break;
        }
        copied += delta ;
        brelse(bh) ;
    } while(copied != len) ;

    printk("%s : In the end, %ld bytes have been copied", MOD_NAME, copied) ;

    if (circular_buffer_end >= file_size) {
        circular_buffer_start += copied ;
    } else if (circular_buffer_end + copied >= file_size) {
        circular_buffer_start += circular_buffer_end + copied - file_size ;
    }
    circular_buffer_end += copied ;
    printk("%s: orig_start %lld, orig_end %lld, circular_buffer_start %lld, circular_buffer_end %lld",
        MOD_NAME, orig_start, orig_end, circular_buffer_start, circular_buffer_end) ;

    *off = circular_buffer_end ;

    mutex_unlock(&log_file_mutex) ;

    return copied ;
}

void internal_logfilefs_write(char *payload) {

    long long orig_start, orig_end ;
    size_t copied = 0 ;
    struct buffer_head *bh ;
    long long selected_sector ;
    size_t len ;

    len = strlen(payload) ;
    if (len == 0) {
        return ;
    }

    mutex_lock(&log_file_mutex) ;

    if (mounted == 0) {
        printk("%s : FS not mounted, payload was %s", MOD_NAME, payload) ;
        mutex_unlock(&log_file_mutex) ;
        return ;
    }

    orig_start = circular_buffer_start ;
    orig_end = circular_buffer_end ;


    do {
        long long circular_buffer_pos, pos_index ;
        ssize_t delta = len - copied ;

        circular_buffer_pos = (orig_end + copied) % file_size ;
        pos_index = (circular_buffer_pos) % DEFAULT_BLOCK_SIZE ;
        printk("%s: position in all buffer is %lld", MOD_NAME, pos_index) ;
        if (pos_index + delta > DEFAULT_BLOCK_SIZE) {
            delta = DEFAULT_BLOCK_SIZE - pos_index ;
        }
        selected_sector = (circular_buffer_pos / DEFAULT_BLOCK_SIZE) +2 ;
        printk("%s: which resides in %lld block", MOD_NAME, selected_sector) ;
        bh = (struct buffer_head *)sb_bread(singleton_sb, selected_sector);
        if (!bh) {
            mutex_unlock(&log_file_mutex) ;
            printk("SOAFileKLRM : sb_bread error, logging aborted") ;
            return ;
        }
        memcpy(bh->b_data + pos_index, payload + copied, delta) ;
        copied += delta ;
        brelse(bh) ;
    } while(copied != len) ;

    printk("%s : In the end, %ld bytes have been copied", MOD_NAME, copied) ;

    if (circular_buffer_end >= file_size) {
        circular_buffer_start += copied ;
    } else if (circular_buffer_end + copied >= file_size) {
        circular_buffer_start += circular_buffer_end + copied - file_size ;
    }
    circular_buffer_end += copied ;
    printk("%s: orig_start %lld, orig_end %lld, circular_buffer_start %lld, circular_buffer_end %lld",
        MOD_NAME, orig_start, orig_end, circular_buffer_start, circular_buffer_end) ;


    mutex_unlock(&log_file_mutex) ;

    return ;
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
    .write = logfilefs_write //please implement this function to complete the exercise
};
