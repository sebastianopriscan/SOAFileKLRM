#ifndef STORE_INTERNAL_H
#define STORE_INTERNAL_H

#include <linux/kernel.h>

#define HASH_TABLE_SIZE 100

struct _store_entry {
    spinlock_t lock ;
    struct _store_entry *parent ;
    char dir_name[1024] ;
    struct list_head siblings ;
    struct list_head children ;
    struct _inode_ht *inode ;
    unsigned long children_num ;
} ;
typedef struct _store_entry store_entry ;

typedef struct _inode_ht {
    uint num ;
    struct list_head peers ;
    store_entry *related_entry ;
} inode_ht ;

typedef struct _store_fs {
    dev_t numbers ;
    struct list_head stores ;
    struct list_head inodes[HASH_TABLE_SIZE] ;
} store_fs ;

extern rwlock_t store_lock ;

int setup_inode_store(void) ;

void cleanup_inode_store(void) ;

#endif