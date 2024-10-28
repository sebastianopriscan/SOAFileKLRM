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

int pre_handler (struct kprobe *, struct pt_regs*) {
	printk("klrm : inside pre handler!") ;
    return 0 ;
}

static struct kprobe krp = {
    .pre_handler = pre_handler 
};  

unsigned long do_open_addr ;

unsigned long get_do_open_addr(void) {
	return do_open_addr ;
}

int hook_init(void) {

	int ret;

    printk("klrm: addr before probe is %lu", (unsigned long) krp.addr) ;

	krp.symbol_name = "do_sys_open";
	ret = register_kprobe(&krp);
	if (ret < 0) {
		pr_info("hook init failed, returned %d\n", ret);
		return ret;
	}
	printk("klrm: hook module correctly loaded\n");
    printk("klrm: addr after probe is %lu", (unsigned long) krp.addr) ;
	do_open_addr = (unsigned long) krp.addr ;
	
	return 0;
}// hook_init

void hook_exit(void) {

	unregister_kprobe(&krp);
	//Be carefull, this unregister assumes that none will need to run the hook function after this nodule
	//is unmounted

}// hook_exit