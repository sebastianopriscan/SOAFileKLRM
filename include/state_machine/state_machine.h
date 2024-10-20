#ifndef FILEKLRM_STATE_MACHINE_H
#define FILEKLR_STATE_MACHINE_H

typedef enum _STATE_MACHINE_STATE {
    ON,
    OFF,
    REC_ON,
    REC_OFF
} STATE_MACHINE_STATE ;

void setup_state_machine() ;

STATE_MACHINE_STATE state_machine_get_state() ;

void state_machine_up() ;
void state_machine_down() ;

#endif