#ifndef PATH_STORE_H
#define PATH_STORE_H

#include "include/api/api.h"

typedef enum _path_check_result {
    MATCH_ROOT,
    FULL_MATCH_LEAF,
    FULL_MATCH_DIR,
    SUB_MATCH_LEAF,
    SUB_MATCH_DIR,
    MATCH_INODE,
    MATCH_UNSOLVED
} PATH_CHECK_RESULT ;

int path_store_add(klrm_path *path) ;
int path_store_rm(klrm_path *path) ;
PATH_CHECK_RESULT path_store_check(klrm_path *path) ;

int setup_path_store(void) ;
void cleanup_path_store(void) ;

#endif