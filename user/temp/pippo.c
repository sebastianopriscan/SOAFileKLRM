#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int fd = open("/dev/klrm-api", O_RDWR) ;
    char buff[1024] ;
    memset(buff, 0, 1024) ;
    buff[0] = 'h' ;

    write(fd, buff, 5) ;
    printf("%d\n", fd) ;
}