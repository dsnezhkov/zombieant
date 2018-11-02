#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
__attribute__((constructor)) static void myctor(int argc, char **argv, char* envp[]){

        printf("ctor\n");
}
int main(int argc, char **argv) {
    for (int i=0; i<argc; i++) {
        printf("%s: argv[%d] = '%s'\n", __FUNCTION__, i, argv[i]);
    }
	 setuid(0);
	 setgid(0);
	 system("/bin/bash");
    while (1) ;
    return 0;
}
