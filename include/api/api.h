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
    OPARG     : struct klrm_path
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_path)

    Description : adds a path to protect to the store
*/
#define ADD_PATH 0x00000000

struct _klrm_path {
    char pathName[1024] ;
} ;
typedef struct _klrm_path klrm_path ;

/******** Operation removePath: ********
    OPCODE    : 0b10
    OPMACRO   : RM_PATH
    OPARG     : struct klrm_path
    OPARGTYPE : IN
    ARGSIZE   : sizeof(struct klrm_path)

    Description : removes a path to protect from the store
*/
#define RM_PATH 0x80000000

//struct _klrm_path {
//    char pathName[1024] ;
//} ;

//Lifecycle operations

int setup_api(void) ;
void cleanup_api(void) ;

#endif