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
#include <crypto/hash.h>

#include "reconfig_access_manager/access_manager.h"

extern char hashed_password[512] ;

int check_password(char *password) {
    struct crypto_shash *tfm;
    struct shash_desc desc;
    unsigned char hash[512];
    int ret;

    tfm = crypto_alloc_shash("sha512", 0, 0);
    if (IS_ERR(tfm)) {
        printk(KERN_ERR "Failed to allocate SHA-256 hash transform: %ld\n", PTR_ERR(tfm));
        return -ENOMEM;
    }

    desc.tfm = tfm;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,19,322)    
    desc.flags = 0;
#endif

    ret = crypto_shash_init(&desc);
    if (ret) {
        printk(KERN_ERR "Failed to initialize hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    ret = crypto_shash_update(&desc, (const u8 *)password, strlen(password));
    if (ret) {
        printk(KERN_ERR "Failed to update hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    ret = crypto_shash_final(&desc, hash);
    if (ret) {
        printk(KERN_ERR "Failed to finalize hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    if(memcmp(hash, hashed_password, 512) == 0) {
        return 0;
    }
    
    return 1;
}
