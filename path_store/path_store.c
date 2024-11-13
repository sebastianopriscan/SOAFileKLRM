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
#include "include/oracles/oracles.h"
#include "store_internal.h"

#define MODNAME "SOAFileKLRM"

#define ARGS_SIZE 512
#define EXPLICITED_PATH_MASK 0x8000000000000000

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
            printk("klrm : deepness incremented to %d", deepness) ; \
            if (deepness != j) goto LOOP_LABEL ; \
            else break ; \
        } \
    } \


#define clean_oracle_decree(decree, isDecree) do {\
    if (isDecree) { \
        filp_close(decree->file, NULL) ; \
        kfree(decree) ;\
    } \
} while (0) ; \

static struct list_head allocations = LIST_HEAD_INIT(allocations) ;

static store_entry *root ;


static struct kmem_cache *dir_cache ;

unsigned int argIdxs[ARGS_SIZE] ;

rwlock_t store_lock ;

static inline unsigned int process_path(char *path) {
    unsigned int i, j ;
    int length = strlen(path) ;

    if (path[0] != '/') return UINT_MAX ;

    for (i = 0, j = 0; i < (unsigned int) length; i++) {
        if (path[i] == '/') {
            path[i] = '\0' ;
            if (j == ARGS_SIZE) {
                printk("%s: Error, path tokens length exceeded", MODNAME) ;
                return UINT_MAX ;
            }
            if (i != ((unsigned int) length) -1) {
                argIdxs[j++] = i+1 ;
            }
        }
    }
    for (i = 0; i < j; i++) {
        if (strcmp(".", path + argIdxs[i]) == 0 || strcmp("..", path + argIdxs[i]) == 0)
            return UINT_MAX ;
    }

    return j ;
}

int path_store_add(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    path_decree *resolved ;
    store_entry *child ;
    store_entry *actualCurrent ;
    char *examinated = path->pathName ;
    unsigned int j ;
    unsigned int deepness = 0 ;
    unsigned int isResolved = 0 ;

    resolved = pathname_oracle(path->pathName) ;
    if (resolved != NULL) {
        examinated = resolved->path ;
        isResolved = 1 ;
    }

    j = process_path(examinated) ;
    if (j == UINT_MAX) {
        clean_oracle_decree(resolved, isResolved) ;
        return 1 ;
    }

    printk("klrm : j is %d", j) ;

    write_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_ADD)

    if (deepness != j ) {
        store_entry *newChild ;

        if (strlen(path->pathName + argIdxs[deepness]) == 0) {
            clean_oracle_decree(resolved, isResolved) ;
            return 1 ;
        }
        newChild = kmem_cache_alloc(dir_cache, GFP_KERNEL) ;
        init_store_entry(newChild) ;
        memcpy(newChild->dir_name, path->pathName + argIdxs[deepness],
            strlen(path->pathName + argIdxs[deepness])) ;

        list_add(&newChild->siblings, curr_entry) ;
        list_add(&newChild->allocations, &allocations) ;
        actualCurrent = list_entry(curr_entry, store_entry, children) ;
        newChild->parent = actualCurrent ;
        actualCurrent->children_num++ ;
        goto LOOP_ADD ;
    }

    actualCurrent = container_of(curr_entry, store_entry, children) ;
    actualCurrent->children_num | EXPLICITED_PATH_MASK ;

    store_iterate_add(resolved->file) ;
    
    write_unlock(&store_lock) ;

    clean_oracle_decree(resolved, isResolved) ;
    return 0 ;
}

int path_store_rm(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    unsigned int j ;
    unsigned int deepness = 0 ;
    path_decree *resolved ;
    char *examinated = path->pathName ;
    int isResolved = 0 ;

    resolved = pathname_oracle(path->pathName) ;
    if (resolved != NULL) {
        examinated = resolved->path ;
        isResolved = 1 ;
    }

    j = process_path(path->pathName) ;
    if (j == UINT_MAX) {
        clean_oracle_decree(resolved, isResolved) ;
        return 1 ;
    }
    
    write_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_RM)

    if (deepness == j) {
        store_entry *parent ;
        store_entry *actualCurrent = list_entry(curr_entry, store_entry, children) ;
        if (!(actualCurrent->children_num & EXPLICITED_PATH_MASK)) {
            write_unlock(&store_lock) ;
            clean_oracle_decree(resolved, isResolved) ;
            return 1 ;
        }
        store_iterate_rm(resolved->file) ;

        if (actualCurrent->children_num != 0) {
            actualCurrent->children_num = actualCurrent->children_num & ~EXPLICITED_PATH_MASK ;
        } else {
            do {
                if (actualCurrent == root ||
                    actualCurrent->children_num != 0 || 
                    actualCurrent->children_num & EXPLICITED_PATH_MASK) break;

                parent = actualCurrent->parent ;
                actualCurrent->parent->children_num-- ;
                list_del(&(actualCurrent->siblings)) ;
                kmem_cache_free(dir_cache, actualCurrent) ;
                actualCurrent = parent ;

            } while(1) ;
        }

    } else {
        write_unlock(&store_lock) ;
        clean_oracle_decree(resolved, isResolved) ;
        return 1 ;
    }

    write_unlock(&store_lock) ;
    clean_oracle_decree(resolved, isResolved) ;
    return 0 ;
}

PATH_CHECK_RESULT path_store_check(klrm_path *path) {

    struct list_head *curr_entry = &(root->children) ;
    struct list_head *tmp ;
    store_entry *child ;
    char *examinated = path->pathName ;
    path_decree *resolved ;
    unsigned int j ;
    unsigned int deepness = 0 ;
    int retVal, isResolved = 0 ;

    resolved = pathname_oracle(path->pathName) ;
    if (resolved != NULL) {
        if (check_inode(resolved->device, resolved->inode)) {
            clean_oracle_decree(resolved, 1) ;
            return MATCH_INODE ;
        } else {
            isResolved = 1 ;
        }
        examinated = resolved->path ;
    }

    j = process_path(examinated) ;
    if (j == UINT_MAX) {
        clean_oracle_decree(resolved, isResolved) ;
        return 1 ;
    }

    read_lock(&store_lock) ;

    PATH_SEARCH_LOOP(LOOP_CHK)

    retVal = list_empty(curr_entry) ;

    read_unlock(&store_lock) ;

    if (curr_entry == &(root->children) && retVal == 1) {
        clean_oracle_decree(resolved, isResolved) ;
        return MATCH_ROOT ;
    }
    if (deepness == j) {
        if (retVal == 1) {
            clean_oracle_decree(resolved, isResolved) ;
            return FULL_MATCH_LEAF ;
        } else {
            clean_oracle_decree(resolved, isResolved) ;
            return FULL_MATCH_DIR ;
        }
    } else {
        if (retVal == 1) {
            clean_oracle_decree(resolved, isResolved) ;
            return SUB_MATCH_LEAF ;
        } else {
            clean_oracle_decree(resolved, isResolved) ;
            return SUB_MATCH_DIR ;
        }
    }
}

static void setup_area(void *buffer) {

}

int setup_path_store(void) {

    rwlock_init(&store_lock) ;

    if(setup_inode_store()) {
        return 1 ;
    }

    dir_cache = kmem_cache_create(
        MODNAME"_dirs",
        2048,
        2048,
        SLAB_POISON,
        setup_area
    );

    if (dir_cache == NULL) {
        printk("%s: Unable to allocate kmem dir cache", MODNAME) ;
        cleanup_inode_store() ;
        return 1 ;
    }

    root = kmem_cache_alloc(dir_cache, GFP_KERNEL) ;
    if (root == NULL) {
        kmem_cache_destroy(dir_cache) ;
        cleanup_inode_store() ;
        printk("%s: Unable to get root structure", MODNAME) ;
        return 1 ;
    }

    init_store_entry(root) ;
    root->dir_name[0] = '/' ;
    list_add(&root->allocations, &allocations) ;


    return 0 ;
}

void cleanup_path_store(void) {

    struct list_head *tmp, *prev = NULL ;
    store_entry *toDelete = NULL ;

    write_lock(&store_lock) ;
    cleanup_inode_store() ;

    list_for_each(tmp, &allocations) {
        if (prev != NULL) {
            toDelete = container_of(prev, store_entry, allocations) ;
            list_del(prev) ;
            kmem_cache_free(dir_cache, toDelete) ;
        }
        prev = tmp ;
    }
    if (prev != NULL) {
        toDelete = container_of(prev, store_entry, allocations) ;
        list_del(prev) ;
        kmem_cache_free(dir_cache, toDelete) ;
    }

    kmem_cache_destroy(dir_cache) ;

    write_unlock(&store_lock) ;
}