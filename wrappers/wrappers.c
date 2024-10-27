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

#define MODNAME "SOAFileKLRM"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
#define get(regs)	((struct pt_regs *)&(regs->di))
#else
#define get(regs)	regs
#endif

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);

#define WRAP_CALL(invocation, write_open_check, from_expr, expr_len, interdiction_return) \
    int __wrap_count ; \
    printk("%s: Inside a syscall wrapper", MODNAME) ; \
    if(!write_open_check) { \
        invocation \
    }\
    state_machine_up(ON) ; \
    \
    klrm_path *path = kmalloc(sizeof(klrm_path), GFP_ATOMIC) ; \
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
    if (path_store_check(path)) { \
        kfree(path) ; \
        return interdiction_return ; \
    } \
    kfree(path) ; \
    invocation \


/****************** Open Wrapper Start *************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long (*original_open_addr)(struct pt_regs *) ;
long __x64_sys_klrm_open(struct pt_regs *regs) {
    WRAP_CALL(return original_open_addr(regs) ;,
        (get(regs)->si & O_RDWR || get(regs)->si & O_WRONLY || get(regs)->si & O_APPEND),
        get(regs)->di, strlen(get(regs)->di), -EACCES)
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

    unprotect_memory() ;

    //Open Wrapper Install
    original_open_addr = __sync_val_compare_and_swap(&((unsigned long *)the_syscall_table)[OPEN_IDX],
        ((unsigned long *)the_syscall_table)[OPEN_IDX], sys_klrm_open) ;

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