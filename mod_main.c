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

#include "include/api/api.h"
#include "include/reconfig_access_manager/access_manager.h"
#include "include/state_machine/state_machine.h"

MODULE_AUTHOR("Sebastian Roberto Opriscan <sebastianroberto.opriscan@gmail.com>");
MODULE_DESCRIPTION("This module implements a Kernel Level Reference Monitor to prevent write access \
	 to a selection of files");

#define MODNAME "SOAFileKLRM"

static int klrm_init(void) {
	setup_api() ;
	setup_state_machine() ;

	return 0 ;
}// hook_init

static void  klrm_exit(void) {
	cleanup_api() ;
}// hook_exit

module_init(klrm_init)
module_exit(klrm_exit)
MODULE_LICENSE("GPL");
