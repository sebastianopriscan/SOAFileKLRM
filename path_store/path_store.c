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

#define MODNAME "SOAFileKLRM"

#define ARGS_SIZE 512

#define init_store_entry(ptr) \
do {\
    memset(ptr, 0, sizeof(store_entry)) ; \
    spin_lock_init(&(ptr->lock)) ; \
    ptr->children.prev = &(ptr->children) ; \
    ptr->children.next = &(ptr->children) ; \
} while(0)\



/*
    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    unsigned int j = 0 ;
    unsigned int deepness = 0 ;

    j = process_path(path) ;
*/
#define PATH_SEARCH_LOOP(LOOP_LABEL) \
LOOP_LABEL: \
 \
    list_for_each(tmp, curr_entry) { \
        child = list_entry(tmp, store_entry, siblings) ; \
        store_entry *prntt = list_entry(curr_entry, store_entry, children) ;\
        printk("%s: looping over %s children, now it's %s", MODNAME, prntt->dir_name, child->dir_name) ; \
        if (strcmp(child->dir_name, path->pathName + argIdxs[deepness]) == 0) { \
            curr_entry = &(child->children) ; \
            deepness++ ; \
            goto LOOP_LABEL ; \
        } \
    } \



struct _store_entry {
    spinlock_t lock ;
    char dir_name[1024] ;
    struct list_head siblings ;
    struct list_head children ;
    unsigned char isLeaf ;
} ;
typedef struct _store_entry store_entry ;

store_entry *root ;

static struct kmem_cache *dir_cache ;

unsigned int argIdxs[ARGS_SIZE] ;

rwlock_t store_lock ;

static inline unsigned int process_path(klrm_path *path) {
    unsigned int i, j ;
    int length = strlen(path->pathName) ;

    for (i = 0, j = 0; i < (unsigned int) length; i++) {
        if (path->pathName[i] == '/') {
            path->pathName[i] = '\0' ;
            if (j == ARGS_SIZE) {
                printk("%s: Error, path tokens length exceeded", MODNAME) ;
                return UINT_MAX ;
            }
            argIdxs[j++] = i+1 ;
            if (i == ((unsigned int) length) -1) {
                printk("%s: Error, path is a directory", MODNAME) ;
                return UINT_MAX ;
            }
        }
    }

    return j ;
}

int path_store_add(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    unsigned int j ;
    unsigned int deepness = 0 ;

    j = process_path(path) ;
    if (j == UINT_MAX) return 1 ;

    write_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_ADD)

    if (deepness != j +1) {
        store_entry *newChild ;

        if (list_entry(curr_entry, store_entry, children)->isLeaf) {
            printk("%s: Warning, element of path passed as dir but actually file", MODNAME) ;
            write_unlock(&store_lock) ;
            return 1 ;
        }
        newChild = kmem_cache_alloc(dir_cache, GFP_KERNEL) ;
        init_store_entry(newChild) ;
        memcpy(newChild->dir_name, path->pathName + argIdxs[deepness],
            strlen(path->pathName + argIdxs[deepness])) ;

        list_add(&newChild->siblings, curr_entry) ;
        goto LOOP_ADD ;
    }

    list_entry(curr_entry, store_entry, children)->isLeaf = 1 ;
    
    write_unlock(&store_lock) ;

    return 0 ;
}

int path_store_rm(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    unsigned int j ;
    unsigned int deepness = 0 ;

    j = process_path(path) ;
    if (j == UINT_MAX) return 1 ;
    
    write_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_RM)

    if (deepness == j +1) {
        store_entry *child = list_entry(curr_entry, store_entry, children) ;
        list_del(&(child->siblings)) ;
        kmem_cache_free(dir_cache, child) ;
    } else {
        write_unlock(&store_lock) ;
        return 1 ;
    }

    write_unlock(&store_lock) ;
    return 0 ;
}

int path_store_check(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    unsigned int j ;
    unsigned int deepness = 0 ;

    j = process_path(path) ;
    if (j == UINT_MAX) return 1 ;

    read_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_CHK)

    read_unlock(&store_lock) ;

    return deepness == j+1 ? 1 : 0 ;
}

static void setup_area(void *buffer) {

}

int setup_path_store(void) {

    rwlock_init(&store_lock) ;

    dir_cache = kmem_cache_create(
        MODNAME"_dirs",
        2048,
        2048,
        SLAB_POISON,
        setup_area
    );

    if (dir_cache == NULL) {
        printk("%s: Unable to allocate kmem dir cache", MODNAME) ;
        return 1 ;
    }

    root = kmem_cache_alloc(dir_cache, GFP_KERNEL) ;
    if (root == NULL) {
        kmem_cache_destroy(dir_cache) ;
        printk("%s: Unable to get root structure", MODNAME) ;
        return 1 ;
    }

    init_store_entry(root) ;
    root->dir_name[0] = '/' ;


    return 0 ;
}

struct pointer_stack {
    struct pointer_stack *above ;
    void *pointer ;
} ;

void cleanup_path_store(void) {
    struct pointer_stack *top ;

    write_lock(&store_lock) ;
    write_unlock(&store_lock) ;
}