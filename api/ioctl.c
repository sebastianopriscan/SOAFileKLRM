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
    int state_machine_ret ;

    state_machine_ret = state_machine_enter_rec_on() ;
    if (state_machine_ret == 0) {
        return -EBUSY ;
    }

    printk("klrm: Got state machine in REC_ON mode") ;

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    printk("klrm: managed to check password") ;

    ret = path_store_add(&input->path) ;

    printk("klrm: added entry to path store") ;

    state_machine_rec_exit() ;
    return ret ;
}

ssize_t klrm_path_rm(klrm_input *input) {

    ssize_t ret ;
    int state_machine_ret ;

    state_machine_ret = state_machine_enter_rec_on() ;
    if (state_machine_ret == 0) {
        return -EBUSY ;
    }

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    ret = path_store_rm(&input->path) ;

    state_machine_rec_exit() ;
    return ret ;
}

ssize_t klrm_path_check(klrm_input *input) {

    ssize_t ret ;
    int state_machine_ret ;

    state_machine_ret = state_machine_enter_rec_on() ;
    if (state_machine_ret == 0) {
        return -EBUSY ;
    }

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    ret = path_store_check(&input->path) ;

    state_machine_rec_exit() ;

    switch(ret) {
        case FULL_MATCH_LEAF:
            printk("SOAFileKLRM : FULL MATCH LEAF") ;
            break;
        case FULL_MATCH_DIR:
            printk("SOAFileKLRM : FULL MATCH DIR") ;
            break;
        case SUB_MATCH_LEAF:
            printk("SOAFileKLRM : SUB MATCH LEAF") ;
            break;
        case SUB_MATCH_DIR:
            printk("SOAFileKLRM : SUB MATCH DIR") ;
            break;
        default :
            printk("SOAFileKLRM : MATCH ROOT") ;
            break;
    }

    return ret ;
}

ssize_t klrm_set(klrm_input *input) {

    ssize_t ret ;

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    if(strcmp(input->path.pathName, "on") == 0) {
        set_machine_on() ;
        return 0 ;
    } else if(strcmp(input->path.pathName, "off") == 0) {
        set_machine_off() ;
        return 0 ;
    } else {
        return -EINVAL ;
    }

}

ssize_t klrm_rec(klrm_input *input) {

    ssize_t ret ;

    if (check_password(input->password) != 0) {
        return -EACCES ;
    }

    if(strcmp(input->path.pathName, "on") == 0) {
        set_machine_rec_on() ;
        return 0 ;
    } else if(strcmp(input->path.pathName, "off") == 0) {
        set_machine_rec_off() ;
        return 0 ;
    } else {
        return -EINVAL ;
    }

}