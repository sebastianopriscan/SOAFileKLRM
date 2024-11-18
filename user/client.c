#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define ADD_PATH 0x00000000
#define RM_PATH 0x20000000
#define CHK_PATH 0x40000000
#define MACHINE_SET 0x60000000
#define MACHINE_REC 0x80000000

typedef struct {
    char path[4096] ;
} klrm_path ;

typedef struct {
    char password[128] ;
    klrm_path path ;
} klrm_input ;

int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Usage: klrmctl add/rm/check/rec/set path/on/off\n") ;
    }

    klrm_input *input = malloc(sizeof(klrm_input)) ;
    memset(input, 0, sizeof(klrm_input)) ;

    printf("Please insert the reference monitor password\n") ;
    fflush(stdout) ;
    unsigned char read = 0 ;
    int res ;
    int idx = 0 ;
    do {
        res = getc(stdin) ;
        if (res == EOF) {
            fprintf(stderr, "EOF reached while reading input, exiting...\n") ;
            return 1 ;
        } 
        read = (unsigned char) res ;
        input->password[idx++] = read ;
        putc('\b', stdout) ;
        fflush(stdout) ;
    } while(read != '\n' || idx == 127) ;
    if(read == '\n')
        input->password[idx -1] = '\0' ;
    else 
        input->password[idx] = '\0' ;

    strcpy(input->path.path, argv[2]) ;

    int fd = open("/dev/klrm-api", O_RDWR) ;
    if (fd == -1) {
        perror("Error opening file:") ;
        return 1 ;
    }

    if (strcmp(argv[1], "add") == 0) {
        fprintf(stderr, "Inside add path\n") ;
        int retval = ioctl(fd, ADD_PATH | (unsigned int) sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }

    if (strcmp(argv[1], "rm") == 0) {
        fprintf(stderr, "Inside rm path\n") ;
        int retval = ioctl(fd, RM_PATH | (unsigned int) sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }

    if (strcmp(argv[1], "check") == 0) {
        fprintf(stderr, "Inside check path\n") ;
        int retval = ioctl(fd, CHK_PATH | (unsigned int) sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }

    if (strcmp(argv[1], "rec") == 0) {
        fprintf(stderr, "Inside rec path\n") ;
        if(strcmp(input->path.path, "on") != 0 || strcmp(input->path.path, "off") != 0) {
            fprintf(stderr, "Usage : klrmctl rec on/off\n") ;
            return 1 ;
        }
        int retval = ioctl(fd, MACHINE_REC | (unsigned int) sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }

    if (strcmp(argv[1], "check") == 0) {
        fprintf(stderr, "Inside set path\n") ;
        if(strcmp(input->path.path, "on") != 0 || strcmp(input->path.path, "off") != 0) {
            fprintf(stderr, "Usage : klrmctl set on/off\n") ;
            return 1 ;
        }
        int retval = ioctl(fd, MACHINE_SET | (unsigned int) sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }
}