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

#include "include/state_machine/state_machine.h"

struct rw_semaphore rec_sem, set_sem ;

typedef enum {
    ON,
    OFF
} TOGGLE ;

TOGGLE rec ;
TOGGLE set ;

void setup_state_machine(void) {
    sema_init(&rec_sem, 0) ;
    sema_init(&set_sem, 0) ;
}

void cleanup_state_machine(void) {

}

int state_machine_enter_on(void) {
    down_read(&set_sem) ;
    if (set == ON) {
        return 1 ;
    } else {
        up_read(&set_sem) ;
        return 0 ;
    }
}

void state_machine_exit(void) {
    up_read(&set_sem) ;
}

int state_machine_enter_rec_on(void) {
    down_read(&rec_sem) ;
    if (rec == ON) {
        return 1 ;
    } else {
        up_read(&rec_sem) ;
        return 0 ;
    }
}

void state_machine_rec_exit(void) {
    up_read(&rec_sem) ;
}

void set_machine_rec_on(void) {
    down_write(&rec_sem) ;
    rec = ON ;
    up_write(&rec_sem) ;
}

void set_machine_rec_off(void) {
    down_write(&rec_sem) ;
    rec = OFF ;
    up_write(&rec_sem) ;
}

void set_machine_on(void) {
    down_write(&set_sem) ;
    set = ON ;
    up_write(&set_sem) ;
}

void set_machine_off(void) {
    down_write(&set_sem) ;
    set = OFF ;
    up_write(&set_sem) ;
}