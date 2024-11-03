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
#include <linux/kernel_read_file.h>
#include <crypto/hash.h>

#include "include/logger/logger.h"
#include "include/logfs/write_handle.h"

typedef struct _logging_work {
    pid_t tgid ;
    pid_t pid ;
    kuid_t uid ;
    kuid_t euid ;
    char *fullpathname ;
    char *pathname ;
    struct work_struct work_ref ;

} logging_work ;

int start_logger(void) {
    return 0 ;
}

void stop_logger(void) {

}

void decompress_hash(unsigned char *compressed_hash, char *buffer) {
    char hash_printer_buffer[129] ;
    int i ; int j ;
    unsigned char compressed_i_lo ;
    char compressed_i_hi ;
    hash_printer_buffer[128] = '\0' ;
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
    sprintf(buffer, "%s\n", hash_printer_buffer) ;
}

static void do_log_work(unsigned long data) {
    logging_work *payload = container_of((void *)data, logging_work, work_ref) ;
    char *log ;
    void *contentbuff ;
    int readTot ;
    size_t filesize ;

    struct crypto_shash *tfm;
    //TODO: CHECK WITH USING KMALLOC IN CSE OF CHANGES IN CIPHER/CIPHER INTERNALS
    char desc[sizeof(struct shash_desc) + 208];
    unsigned char hash[128];
    int ret;

    log = kzalloc(4096+2048, GFP_KERNEL) ;
    if (log == NULL) {
        printk("SOAFileKLRM : Unable to process deferred work, aborting") ;
        goto FREE_INPUT ;
    }

    sprintf(log,"TGID:%d;PID:%d;UID:%d;EUID:%d;exe:%s;hash:",
        payload->tgid, payload->pid, payload->uid.val, payload->euid.val, payload->pathname) ;

    printk("klrm : inside do log work, already prepared logging work base: %s", log) ;

    contentbuff = kzalloc(65536, GFP_KERNEL) ;
    if (contentbuff == NULL) {
        printk("SOAFileKLRM : Unable to allocate page for dumping file") ;
        kfree(log) ;
        goto FREE_INPUT ;
    }

    readTot = kernel_read_file_from_path(payload->pathname, 0, &contentbuff, 65536, &filesize, READING_UNKNOWN) ;
    if (readTot < 0) {
        printk("SOAFileKLRM : Unable to open file %s for dumping", payload->pathname) ;
        sprintf(log + strlen(log), "unavailable\n") ;
        goto DO_DUMP ;
    }
    if (readTot != filesize) {
        printk("SOAFileKLRM : Unable to dump full file %s, only %d out of %d, doing only partial hash",
        payload->pathname, readTot, filesize) ;
    }
    printk("klrm : ran kernel_read_file_from_path, contentbuff is %px", contentbuff) ;
    
    tfm = crypto_alloc_shash("sha512", 0, 0);
    if (IS_ERR(tfm)) {
        kfree(contentbuff) ;
        kfree(log) ;
        printk("Failed to allocate SHA-512 hash transform: %ld\n", PTR_ERR(tfm));
        goto FREE_INPUT ;
    }

    ((struct shash_desc*)desc)->tfm = tfm;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,19,322)    
    desc.flags = 0;
#endif

    ret = crypto_shash_init((struct shash_desc *)desc);
    if (ret) {
        printk("Failed to initialize hash: %d\n", ret);
        crypto_free_shash(tfm);
        kfree(contentbuff) ;
        kfree(log) ;
        goto FREE_INPUT ;
    }

    ret = crypto_shash_update((struct shash_desc *)desc, contentbuff, readTot);
    if (ret) {
        printk("Failed to update hash: %d\n", ret);
        crypto_free_shash(tfm);
        kfree(contentbuff) ;
        kfree(log) ;
        goto FREE_INPUT ;
    }

    printk("klrm : setup hash") ;

    ret = crypto_shash_final((struct shash_desc *)desc, hash);
    if (ret) {
        printk("Failed to finalize hash: %d\n", ret);
        crypto_free_shash(tfm);
        kfree(contentbuff) ;
        kfree(log) ;
        goto FREE_INPUT ;
    }

    printk("klrm : executed hash") ;

    decompress_hash(hash, log + strlen(log)) ;

    printk("klrm : executed hash") ;

    crypto_free_shash(tfm) ;
    kfree(contentbuff) ;

DO_DUMP:
    internal_logfilefs_write(log) ;
    printk("SOAFileKLRM : dumped log is %s", log) ;
    kfree(log) ;
FREE_INPUT:
    kfree(payload->fullpathname) ;
    kfree((void *) payload) ;
    module_put(THIS_MODULE) ;
}

void log_append(void) {

    logging_work *work ;
    struct path *path ;
    char *fullpathname ;
    char *pathname ;

    if(!try_module_get(THIS_MODULE)) {
        printk("SOAFileKLRM : Unable to lock module for deferred logging work, log aborted") ;
        return ;
    }

    work = kzalloc(sizeof(logging_work), GFP_KERNEL) ;
    if (work == NULL) {
        module_put(THIS_MODULE) ;
        printk("SOAFileKLRM : Unable to allocate deferred work descriptor, log aborted") ;
        return ;
    }

    fullpathname = kzalloc(4096, GFP_KERNEL) ;     
    if (fullpathname == NULL) {
        module_put(THIS_MODULE) ;
        kfree(work) ;
        printk("SOAFileKLRM : Unable to allocate pathname buffer, log aborted") ;
        return ;
    }

    work->pid = current->pid ;
    work->tgid = current->tgid ;
    work->uid = get_current_cred()->uid ;
    work->euid = get_current_cred()->euid ;
    printk("klrm : reached credentials") ;

    //DEBUG module_put(THIS_MODULE) ;
    //DEBUG kfree(fullpathname) ;
    //DEBUG kfree(work) ;
    path = &(current->mm->exe_file->f_path) ;
    printk("klrm : path is %px", path) ;
    //DEBUG module_put(THIS_MODULE) ;
    //DEBUG kfree(fullpathname) ;
    //DEBUG kfree(work) ;

    path_get(path) ;

    pathname = d_path(path, fullpathname, 4096) ;
    if (IS_ERR(pathname)) {
        module_put(THIS_MODULE) ;
        kfree(work) ;
        kfree(fullpathname) ;
        printk("SOAFileKLRM : pathname space was not enough to contain the program path, log aborted") ;
        return ;
    }

    path_put(path) ;

    work->fullpathname = fullpathname ;
    work->pathname = pathname ;

    __INIT_WORK(&(work->work_ref), (void *)do_log_work, (unsigned long)(&work->work_ref)) ;

    printk("klrm : only need to schedule work") ;
    //module_put(THIS_MODULE) ;
    //kfree(fullpathname) ;
    //kfree(work) ;
    schedule_work(&work->work_ref) ;

}