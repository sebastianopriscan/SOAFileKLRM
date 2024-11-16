#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/printk.h>      
#include <linux/ptrace.h>       
#include <linux/syscalls.h>
#include <linux/version.h>

#include "include/path_store/path_store.h"
#include "store_internal.h"

#define MODNAME "SOAFileKLRM"

static struct kmem_cache *fs_cache ;
static struct kmem_cache *inodes_cache ;

struct list_head fs_stores = LIST_HEAD_INIT(fs_stores) ;


static inline int inode_allocate(store_fs * store, unsigned long inode) {

    struct list_head *tmp ;
    inode_ht *node ;

    list_for_each(tmp, &store->inodes[inode % HASH_TABLE_SIZE]) {
        node = container_of(tmp, inode_ht, peers) ;
        if (node->num == inode) {

            if (node->stamp < nonce) {
                node->refCount++ ;
                node->stamp = nonce ;
                return 1 ;
            }

            return -1 ;
        }
    }

    node = kmem_cache_alloc(inodes_cache, GFP_KERNEL) ;
    if (node == NULL) {
        printk("%s : Error allocating new node, aborting add", MODNAME) ;
        return 0 ;
    }
    node->num = inode ;
    node->refCount++ ;
    node->stamp = nonce ;
    list_add(&node->peers, &(store->inodes[inode % HASH_TABLE_SIZE])) ;
    return 1 ;
}

int insert_inode_ht(dev_t table, unsigned long inode) {

    struct list_head *tmp ;
    store_fs *store ;
    int i ;

    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;
        if (store->numbers == table) {
            return inode_allocate(store, inode) ;
        }
    }

    store = kmem_cache_alloc(fs_cache, GFP_KERNEL) ;
    if (store == NULL) {
        printk("%s : Error allocating new fs cache, aborting add", MODNAME) ;
        return 0 ;
    }
    store->stores.next = &store->stores ;
    store->stores.prev = &store->stores ;
    for (i = 0 ; i < HASH_TABLE_SIZE ; i++) {
        store->inodes[i].next = &store->inodes[i] ;
        store->inodes[i].prev = &store->inodes[i] ;
    }

    list_add(&store->stores, &fs_stores) ;
    return inode_allocate(store, inode) ;    
}

static inline void inode_deallocate(inode_ht *node) {
    if (node->refCount != 0) return ;
    list_del(&node->peers) ;
    kmem_cache_free(inodes_cache, node) ;
}

int rm_inode_ht(dev_t table, unsigned long inode) {

    struct list_head *tmp ;
    store_fs *store ;
    inode_ht *node ;

    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;

        if (store->numbers == table) {
            list_for_each(tmp, &store->inodes[inode % HASH_TABLE_SIZE]) {
                node = container_of(tmp, inode_ht, peers) ;
                if (node->num == inode) {

                    if (node->stamp < nonce) {
                        node->refCount-- ;
                        node->stamp = nonce ;
                        inode_deallocate(node) ;
                        return 1 ;
                    }

                    return -1 ;
                }
            }
        }
    }
    return 0 ;
}

int check_inode(dev_t table, unsigned long inode) {

    struct list_head *tmp, *tmp2 ;
    store_fs *store ;
    inode_ht *node ;

    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;
        if (store->numbers == table) {
            list_for_each(tmp2, &store->inodes[inode % HASH_TABLE_SIZE]) {
                node = container_of(tmp2, inode_ht, peers) ;
                if (node->num == inode) {
                    return 1 ;
                }
            }
        }
    }
    return 0 ;
}

static void setup_area(void *buffer) {

}

int setup_inode_store(void) {

    fs_cache = kmem_cache_create("SOAFileKLRM-fs", sizeof(store_fs), 0, SLAB_POISON, setup_area) ;
    if (fs_cache == NULL) {
        printk("SOAFileKLRM : Error setupping fs cache") ;
        return 1 ;
    }

    inodes_cache = kmem_cache_create("SOAFileKLRM-ino", sizeof(inode_ht), 0, SLAB_POISON, setup_area) ;
    if (inodes_cache == NULL) {
        kmem_cache_destroy(fs_cache) ;
        printk("SOAFileKLRM : Error setupping inode cache") ;
        return 1 ;
    }

    return 0 ;
}

//O(saved_nodes) function, quite heavy, but on practical scenarios might never be executed
void refresh_nonces(unsigned long work_unit) {
    struct list_head *tmp, *tmp2 ;
    store_fs *store ;
    inode_ht *node ;
    int i ;

    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;
        for (i = 0; i < HASH_TABLE_SIZE; i++) {
            list_for_each(tmp2, &store->inodes[i]) {
                node = container_of(tmp2, inode_ht, peers) ;
                node->stamp = 0 ;
            }
        }
    }
    nonce = 0 ;
    module_put(THIS_MODULE) ;
    kfree((void *)work_unit) ;
    return 0 ;
}

void cleanup_inode_store(void) {

    struct list_head *tmp, *itmp ;
    struct list_head *prevNode = NULL, *prevStore = NULL ;
    store_fs *curr, *toDeleteStore ;
    inode_ht *toDelete ;
    int i ;

    list_for_each(tmp, &fs_stores) {
        curr = container_of(tmp, store_fs, stores) ;
        for (i = 0; i < HASH_TABLE_SIZE; i++ ) {
            list_for_each(itmp, &curr->inodes[i]) {
                if (prevNode != NULL) {
                    toDelete = container_of(prevNode, inode_ht, peers) ;
                    list_del(prevNode) ;
                    kmem_cache_free(inodes_cache, toDelete) ;
                }
                prevNode = itmp ;
            }
            toDelete = container_of(prevNode, inode_ht, peers) ;
            list_del(prevNode) ;
            kmem_cache_free(inodes_cache, toDelete) ;
            prevNode = NULL ;
        }
        if (prevStore != NULL) {
            toDeleteStore = container_of(prevStore, store_fs, stores) ;
            list_del(prevStore) ;
            kmem_cache_free(fs_cache, toDeleteStore) ;
        }
        prevStore = tmp ;
    }
    if (prevStore != NULL) {
        toDeleteStore = container_of(prevStore, store_fs, stores) ;
        list_del(prevStore) ;
        kmem_cache_free(fs_cache, toDeleteStore) ;
    }

    kmem_cache_destroy(fs_cache) ;
    kmem_cache_destroy(inodes_cache) ;
}