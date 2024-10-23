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

struct _store_entry {
    klrm_path path ;
    struct list_head node ;
} ;
typedef struct _store_entry store_entry ;

static struct list_head list_endpoints ;
static struct kmem_cache *store_cache ;

int path_store_add(klrm_path *path) {
    return 0 ;
}

int path_store_rm(klrm_path *path) {
    return 0 ;
}

static void setup_area(void *buffer) {

}

int setup_path_store(void) {

    list_endpoints.next = NULL ;
    list_endpoints.prev = NULL ;

    store_cache = kmem_cache_create(
        MODNAME,
        sizeof(store_entry) & (~(0UL) << 11UL ),
        sizeof(store_entry) & (~(0UL) << 11UL ),
        SLAB_POISON,
        setup_area
    );

    if (store_cache == NULL) {
        printk("%s: Unable to allocate kmem cache") ;
        return 1 ;
    }

    return 0 ;
}

void cleanup_path_store(void) {

}