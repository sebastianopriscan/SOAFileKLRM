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

#include "../include/state_machine/state_machine.h"

#define STATE_ON      0x4000000000000000
#define STATE_OFF     0x0
#define STATE_REC_ON  0xb000000000000000
#define STATE_REC_OFF 0x8000000000000000

#define CLEAR_MACHINE 0x3fffffffffffffff

#define get_machine_state(state) \
    do { \
        if (__sync_bool_compare_and_swap(&machine_state_atomic, \
            machine_state_atomic & ~CLEAR_MACHINE, state | 1UL)) \
        break; \
        \
        if (__sync_bool_compare_and_swap(&machine_state_atomic, \
            (machine_state_atomic & CLEAR_MACHINE) | state, machine_state_atomic+1UL)) \
        break; \
    } while (1) \


volatile unsigned long machine_state_atomic = 0UL ;

void setup_state_machine(void) {
}

STATE_MACHINE_STATE state_machine_get_state(void) {
    __sync_fetch_and_add(&machine_state_atomic, 1UL) ;
    switch ((machine_state_atomic & ~CLEAR_MACHINE)) {
        case STATE_ON : {
            return ON ;
        }
        case STATE_OFF : {
            return OFF ;
        }
        case STATE_REC_ON : {
            return REC_ON ;
        }
        case STATE_REC_OFF : {
            return REC_OFF ;
        }
    }
    return OFF ;
}

void state_machine_up(STATE_MACHINE_STATE state) {
    switch (state) {
        case ON :
            get_machine_state(STATE_ON) ;
            break;
        case OFF :
            get_machine_state(STATE_OFF) ;
            break;
        case REC_ON :
            get_machine_state(STATE_REC_ON) ;
            break;
        case REC_OFF :
            get_machine_state(STATE_REC_OFF) ;
            break;
    }
}

void state_machine_down(void) {
    __sync_fetch_and_sub(&machine_state_atomic, 1UL) ;
}
