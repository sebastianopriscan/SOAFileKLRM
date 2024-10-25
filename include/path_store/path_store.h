#ifndef PATH_STORE_H
#define PATH_STORE_H

#include "include/api/api.h"

int path_store_add(klrm_path *path) ;
int path_store_rm(klrm_path *path) ;
int path_store_check(klrm_path *path) ;

int setup_path_store(void) ;
void cleanup_path_store(void) ;

#endif