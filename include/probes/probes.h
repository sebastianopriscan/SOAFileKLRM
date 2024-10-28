#ifndef HOOKS_H
#define HOOKS_H

unsigned long get_do_open_addr(void) ;

int hook_init(void) ;
void hook_exit(void) ;

#endif