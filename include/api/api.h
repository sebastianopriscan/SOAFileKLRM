#ifndef API_H
#define API_H

/*
ioctl op arg specification:

The op arg uses the two most significant bits to encode the operation kind,
for each operation it will be specified if it's in and/or out only here,
and the parameters'length will be passed in the remaining byte
*/

/******** Operation addPath: ********
    OPCODE    : 0b00
    OPMACRO   : ADD_PATH
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_path)

    Description : adds a path to protect to the store
*/
#define ADD_PATH 0x00000000

struct _klrm_path {
    char pathName[4096] ;
} ;
typedef struct _klrm_path klrm_path ;

struct _klrm_input {
    char password[128] ;
    klrm_path path ;
} ;
typedef struct _klrm_input klrm_input ;

ssize_t klrm_path_add(klrm_input *) ;

/******** Operation removePath: ********
    OPCODE    : 0b10
    OPMACRO   : RM_PATH
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_input)

    Description : removes a path to protect from the store
*/
#define RM_PATH 0x80000000

/*
struct _klrm_path {
    char pathName[4096] ;
} ;

struct _klrm_input {
    char password[128] ;
    klrm_path path ;
} ;
*/

ssize_t klrm_path_rm(klrm_input *) ;


/*********** Lifecycle operations **********/

int setup_api(void) ;
void cleanup_api(void) ;

#endif