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

#include "include/oracles/oracles.h"

unsigned long addr_kprobe_oracle(char *symbol) {

	int ret;
	unsigned long addr ;
	struct kprobe kp ;

	memset(&kp, 0, sizeof(struct kprobe)) ;

    printk("klrm: addr before probe is %lu", (unsigned long) kp.addr) ;

	kp.symbol_name = symbol ;
	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_info("hook init failed, returned %d\n", ret);
		return NULL;
	}
    printk("klrm: addr after probe is %lu", (unsigned long) kp.addr) ;
	addr = (unsigned long) kp.addr ;

	unregister_kprobe(&kp) ;
	
	return addr;
}
