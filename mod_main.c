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
#include "include/path_store/path_store.h"
#include "include/wrappers/wrappers.h"
#include "include/oracles/oracles.h"

MODULE_AUTHOR("Sebastian Roberto Opriscan <sebastianroberto.opriscan@gmail.com>");
MODULE_DESCRIPTION("This module implements a Kernel Level Reference Monitor to prevent write access \
	 to a selection of files");

#define MODNAME "SOAFileKLRM"

static int klrm_init(void) {
	setup_state_machine() ;
	setup_password() ;
	setup_wrappers() ;
	if(setup_path_store() != 0) {
		return 1 ;
	}
	if(setup_api() != 0 ) {
		cleanup_path_store() ;
		return 1 ;
	}

	return 0 ;
}

static void  klrm_exit(void) {
	cleanup_wrappers() ;
	cleanup_path_store() ;
	cleanup_api() ;
}

module_init(klrm_init)
module_exit(klrm_exit)
MODULE_LICENSE("GPL");
