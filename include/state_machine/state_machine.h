#ifndef FILEKLRM_STATE_MACHINE_H
#define FILEKLRM_STATE_MACHINE_H

void setup_state_machine(void) ;
void cleanup_state_machine(void) ;

void set_machine_on(void) ;
void set_machine_off(void) ;
int state_machine_enter_on(void) ;
void state_machine_exit(void) ;

void set_machine_rec_on(void) ;
void set_machine_rec_off(void) ;
int state_machine_enter_rec_on(void) ;
void state_machine_rec_exit(void) ;

#endif