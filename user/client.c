#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>

typedef struct {
    char path[4096] ;
} klrm_path ;

typedef struct {
    char password[128] ;
    klrm_path path ;
} klrm_input ;

int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: klrmctl add/rm password path") ;
    }

    klrm_input *input = malloc(sizeof(klrm_input)) ;

    strcpy(input->password, argv[2]) ;
    strcpy(input->path.path, argv[3]) ;

    int fd = open("/dev/klrm-api", O_RDWR) ;
    if (fd == -1) {
        perror("Error opening file:") ;
        return 1 ;
    }

    if (strcmp(argv[1], "add") == 0) {
        int retval = ioctl(fd, sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }

    if (strcmp(argv[1], "rm") == 0) {
        int retval = ioctl(fd, 0x80000000 | sizeof(klrm_input), input) ;
        fprintf(stderr, "ioctl returned %d\n", retval) ;
        return 0 ;
    }
}