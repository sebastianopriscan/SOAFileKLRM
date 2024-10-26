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
#include <linux/fdtable.h>
#include <linux/fs_struct.h>

#include "include/api/api.h"
#include "include/reconfig_access_manager/access_manager.h"
#include "include/state_machine/state_machine.h"
#include "include/path_store/path_store.h"

ssize_t klrm_path_add(klrm_input *input) {

    ssize_t ret ;

    state_machine_up(REC_ON) ;

    printk("klrm: Got state machine in REC_ON mode") ;

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    printk("klrm: managed to check password") ;

    ret = path_store_add(&input->path) ;

    printk("klrm: added entry to path store") ;

    state_machine_down() ;
    return ret ;
}

ssize_t klrm_path_rm(klrm_input *input) {

    ssize_t ret ;

    state_machine_up(REC_ON) ;

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    ret = path_store_rm(&input->path) ;

    state_machine_down() ;
    return ret ;
}