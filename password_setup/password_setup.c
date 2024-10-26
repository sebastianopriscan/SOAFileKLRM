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

#include "include/reconfig_access_manager/access_manager.h"

extern char hashed_password[129] ;
char hash_compressed[64] ;

char hash_printer_buffer[129] ;
void print_decompressed_hash(unsigned char *compressed_hash) {
    int i ; int j ;
    unsigned char compressed_i_lo ;
    char compressed_i_hi ;
    j = 0;
    for(i = 0; i < 64; i++) {
        compressed_i_hi = compressed_hash[i] >> ((unsigned char)4) ;
        compressed_i_lo = compressed_hash[i] & ((unsigned char)0x0f) ;
        
        if (compressed_i_hi <= 9) {
            hash_printer_buffer[j] = compressed_i_hi + 48;
        } else {
            hash_printer_buffer[j] = compressed_i_hi + 87 ;
        }
        
        if (compressed_i_lo <= 9) {
            hash_printer_buffer[j+1] = compressed_i_lo + 48;
        } else {
            hash_printer_buffer[j+1] = compressed_i_lo + 87 ;
        }
        j+=2 ;
    }
    printk("klrm: Evaluated hash is: %s", hash_printer_buffer) ;
}

int setup_password(void) {
    int i ;
    int j ;
    unsigned char hashChunk ;
    unsigned char charToTransform ;

    if (strlen(hashed_password) != 128) {
        printk("klrm: Error: provided password hash is incorrect") ;
        return 1 ;
    }

    for(i = 0, j = 0; i < 128; i+=2 , j++) {
        hashChunk = 0 ;
        
        if (48 <= hashed_password[i] && hashed_password[i] <= 57) {
            charToTransform = hashed_password[i] - 48 ;
        } else {
            charToTransform = hashed_password[i] - 97 +10;
        }
        hashChunk |= (charToTransform) << ((unsigned char) 4) ;

        if (48 <= hashed_password[i+1] && hashed_password[i+1] <= 57) {
            charToTransform = hashed_password[i+1] - 48 ;
        } else {
            charToTransform = hashed_password[i+1] - 97 +10 ;
        }
        hashChunk |= charToTransform ;

        hash_compressed[j] = hashChunk ;
    }

    return 0;
}

int check_password(char *password) {

    struct crypto_shash *tfm;
    //TODO: CHECK WITH USING KMALLOC IN CSE OF CHANGES IN CIPHER/CIPHER INTERNALS
    char desc[sizeof(struct shash_desc) + 208];
    unsigned char hash[128];
    int ret;
    
    printk("klrm: Entered check password") ;

    tfm = crypto_alloc_shash("sha512", 0, 0);
    if (IS_ERR(tfm)) {
        printk("Failed to allocate SHA-512 hash transform: %ld\n", PTR_ERR(tfm));
        return -ENOMEM;
    }

    ((struct shash_desc*)desc)->tfm = tfm;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,19,322)    
    desc.flags = 0;
#endif

    printk("klrm: ran crypto_alloc_shash") ;
    printk("klrm: size of desc for sha512 is: %d", crypto_shash_descsize(tfm)) ;
    //DEBUG crypto_free_shash(tfm) ;

    ret = crypto_shash_init((struct shash_desc *)desc);
    if (ret) {
        printk("Failed to initialize hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    printk("klrm: ran crypto_shash_init") ;
    //DEBUG crypto_free_shash(tfm) ;


    ret = crypto_shash_update((struct shash_desc *)desc, (const u8 *)password, strlen(password));
    if (ret) {
        printk("Failed to update hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    printk("klrm: ran crypto_shash_update") ;
    //DEBUG crypto_free_shash(tfm) ;

    ret = crypto_shash_final((struct shash_desc *)desc, hash);
    if (ret) {
        printk("Failed to finalize hash: %d\n", ret);
        crypto_free_shash(tfm);
        return -1 ;
    }

    if(memcmp(hash, hash_compressed, 64) != 0) {
        printk("klrm: Hashes are not the same") ;
        printk("klrm: Saved hash is %s", hashed_password) ;
        print_decompressed_hash(hash) ;
        return 1;
    }
    
    printk("klrm: Hashes compared successfully") ;
    crypto_free_shash(tfm) ;

    return 0;
}
