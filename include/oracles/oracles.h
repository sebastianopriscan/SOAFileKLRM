#ifndef ORACLE_H
#define ORACLE_H

unsigned long addr_kprobe_oracle(char *) ;

typedef struct __path_decree {
    char *path ;
    dev_t device ;
    unsigned long inode ;
    struct path path_struct ;
} path_decree ;

path_decree *pathname_oracle(char *) ;
path_decree *pathname_oracle_at(int dirfd, char *path) ;

#endif