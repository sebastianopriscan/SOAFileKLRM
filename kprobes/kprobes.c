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

#include "include/kprobes/kprobes.h"

#define MODNAME "SOAFileKLRM"

extern int create_open_jprobe() ;
extern int destroy_open_jprobe() ;

int setup_kprobes() {

}

void cleanup_kprobes() {

}