#include <stdio.h>
#include <unistd.h>


__attribute__((constructor)) static void protect_ctor(int argc, char **argv, char* envp[]){

        printf("in protect ctor\n");
}

int main(int argc, char **argv) {

    while (1) { 
		
        printf("sleeping... \n");
		sleep(1) ; 
 	}
    return 0;
}
