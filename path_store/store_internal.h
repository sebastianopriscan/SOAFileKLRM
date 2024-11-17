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
    struct list_head allocations ;
    unsigned long children_num ;
} ;
typedef struct _store_entry store_entry ;

typedef struct _inode_ht {
    uint num ;
    unsigned long refCount ;
    unsigned long long stamp ;
    struct list_head peers ;
} inode_ht ;

typedef struct _store_fs {
    dev_t numbers ;
    struct list_head stores ;
    struct list_head inodes[HASH_TABLE_SIZE] ;
} store_fs ;

extern rwlock_t store_lock ;
extern unsigned long long nonce ;

int setup_inode_store(void) ;
void cleanup_inode_store(void) ;

int insert_inode_ht(dev_t, unsigned long) ;
int rm_inode_ht(dev_t, unsigned long) ;

int check_inode(dev_t, unsigned long) ;

void refresh_nonces(unsigned long) ;

void store_iterate_add(struct path *) ;
void store_iterate_rm(struct path *) ;

#endif