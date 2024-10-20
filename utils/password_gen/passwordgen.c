//INVOKE BY RUNNING `make gen-password` IN THE ROOT'S MAIN PROJECT!

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <crypt.h>
#include <sys/random.h>
#include <string.h>

struct crypt_data data ;
char salt[64] ;

char *file = "char password_hashed[384] = \"%s\";\n" ;

int main(void) {

    memset(&data, 0, sizeof(struct crypt_data)) ;
    
    printf("Please insert a password for the module:\n") ;

    while(fgets(data.input, 512, stdin) == NULL) {
        printf("Please, insert the password:\n") ;
    }

    int read = 0 ;

    do {
        read += getrandom(salt + read, 64 - read, GRND_RANDOM) ;
    } while (read != 64) ;

    char *setting_buf = crypt_gensalt("$1$", 10, "pippopippopippopippo", strlen("pippopippopippopippo")) ;
    memcpy(data.setting, setting_buf, strlen(setting_buf)) ;


    crypt_r(data.input, data.setting, &data) ;

    FILE *node = fopen("./password_setup/password.c", "w+") ;
    if (node == NULL) {
        perror("fopen error, details:") ;
        exit(1) ;
    }

    struct crypt_data data2 ;
    memset(&data2, 0, sizeof(struct crypt_data)) ;

    memcpy(data2.setting, "$y$jET$", strlen("$y$jET$")) ;
    memcpy(data2.input, data.setting+7, strlen(data.setting +7)) ;
    memcpy(data2.input +strlen(data.setting +7), data.input, strlen(data.input)) ;

    crypt_r(data2.input, data2.setting, &data2) ;

    printf("Setting 1: %s\n", data.setting) ;
    printf("Output 2: %s", data2.input) ;

    printf("Output 1: %s\n", data.output) ;
    printf("Output 2: %s\n", data2.output) ;

    fprintf(node, file, data.output) ;
}