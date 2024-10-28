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

#include "include/wrappers/wrappers.h"
#include "include/state_machine/state_machine.h"
#include "include/path_store/path_store.h"
#include "include/scth/scth.h"
#include "include/probes/probes.h"

#define MODNAME "SOAFileKLRM"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
#define get(regs)	((struct pt_regs *)&(regs->di))
#else
#define get(regs)	regs
#endif

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);

#define WRAP_CALL(invocation, write_open_check, from_expr, expr_len, interdiction_return) \
    printk("Wrapping open") ; \
    if(!write_open_check) { \
        invocation \
    }\
    \
    klrm_path *path = kmalloc(sizeof(klrm_path), GFP_KERNEL) ; \
    printk("klrm: Kmlloc'd") ; \
    if (path == NULL) { \
        printk("%s: Error allocating path to check with kmalloc", MODNAME) ; \
        kfree(path) ; \
        return interdiction_return ; \
    } \
    if (copy_from_user(path->pathName, from_expr, expr_len) != 0) { \
        printk("%s: Error copying data from user", MODNAME) ; \
        kfree(path) ; \
        return interdiction_return ; \
    } \
    kfree(path) ; \
    invocation \


/****************** Open Wrapper Start *************************/
unsigned long original_open_addr ;
long (*do_sys_open_addr)(int, const char __user *, int, umode_t) ;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(3, _klrm_open, char *, filename, int, flags, int, mode) {
    WRAP_CALL(if (force_o_largefile()) flags |= O_LARGEFILE; return do_sys_open_addr(AT_FDCWD, filename, flags, mode);,
        (flags & O_RDWR || flags & O_WRONLY || flags & O_APPEND),
        filename, strlen(filename), -EACCES)
}
#else
long (*original_open_addr)(const char *, int, int) ;
asmlinkage long sys_klrm_open(const char * filename, int flags, int mode) {
    WRAP_CALL(__x64_sys_open_addr(regs),
        (flags & O_RDWR || flags & O_WRONLY || flags & O_APPEND),
        filename, strlen(filename), -EACCES)
}
#endif

#define OPEN_IDX 2

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
unsigned long sys_klrm_open = (unsigned long) __x64_sys_klrm_open ;
#endif
/****************** Open Wrapper End *************************/

int setup_wrappers(void) {

    if (the_syscall_table == 0x0){
	   printk("%s: cannot manage sys_call_table address set to 0x0\n",MODNAME);
	   return -1;
	}
    
    do_sys_open_addr = (long (*)(int, const char *, int, umode_t))(get_do_open_addr()) ;

    unprotect_memory() ;

    //Open Wrapper Install
    printk("%s : original open address is: %ld", MODNAME, original_open_addr) ;
    original_open_addr = __sync_val_compare_and_swap(&((unsigned long *)the_syscall_table)[OPEN_IDX],
        ((unsigned long *)the_syscall_table)[OPEN_IDX], sys_klrm_open) ;
    printk("%s : original open address is: %ld", MODNAME, original_open_addr) ;

    protect_memory() ;

    return 0 ;

}

void cleanup_wrappers(void) {

    unprotect_memory() ;

    //Open Wrapper Uninstall
    original_open_addr = __sync_val_compare_and_swap(&((unsigned long *)the_syscall_table)[OPEN_IDX],
        ((unsigned long *)the_syscall_table)[OPEN_IDX], original_open_addr) ;

    protect_memory() ;
}