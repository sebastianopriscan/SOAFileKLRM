#ifndef API_H
#define API_H

/*
ioctl op arg specification:

The op arg uses the three most significant bits to encode the operation kind,
for each operation it will be specified if it's in and/or out only here,
and the parameters'length will be passed in the remaining bytes
*/

/******** Operation addPath: ********
    OPCODE    : 0b000
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
    OPCODE    : 0b001
    OPMACRO   : RM_PATH
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_input)

    Description : removes a path to protect from the store
*/
#define RM_PATH 0x20000000

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

/******** Operation checkPath: ********
    OPCODE    : 0b010
    OPMACRO   : CHK_PATH
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_input)

    Description : checks a path's status in the store
*/
#define CHK_PATH 0x40000000

/*
struct _klrm_path {
    char pathName[4096] ;
} ;

struct _klrm_input {
    char password[128] ;
    klrm_path path ;
} ;
*/

/******** Operation setMachine: ********
    OPCODE    : 0b011
    OPMACRO   : MACHINE_SET
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_input)

    Description : sets the machine on/off state
*/
#define MACHINE_SET 0x60000000

/*
struct _klrm_path {
    char pathName[4096] ;
} ;

struct _klrm_input {
    char password[128] ;
    klrm_path path ;
} ;
*/
ssize_t klrm_set(klrm_input *) ;

/******** Operation recMachine: ********
    OPCODE    : 0b100
    OPMACRO   : MACHINE_REC
    OPARG     : struct klrm_input
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_input)

    Description : sets the machine on/off state
*/
#define MACHINE_REC 0x80000000

/*
struct _klrm_path {
    char pathName[4096] ;
} ;

struct _klrm_input {
    char password[128] ;
    klrm_path path ;
} ;
*/
ssize_t klrm_rec(klrm_input *) ;

/*********** Lifecycle operations **********/

int setup_api(void) ;
void cleanup_api(void) ;

#endif