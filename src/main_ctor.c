#include <stdio.h>
#include <unistd.h>


__attribute__((constructor (101))) static void protect_ctor1(int argc, char **argv, char* envp[]){

        printf("in protect ctor 1\n");
}
__attribute__((constructor (102))) static void protect_ctor2(int argc, char **argv, char* envp[]){

        printf("in protect ctor 2\n");
}

int main(int argc, char **argv) {

	short c=0;
    for(;c<3;c++) { 
		
        printf("sleeping... \n");
		sleep(1) ; 
 	}
    return 0;
}
