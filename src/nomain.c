#include <stdio.h>
#include <stdlib.h>

// gcc -nostartfiles -o nomain nomain.c 
/* when called :
		$ LD_PRELOAD="./nomain" /bin/cat 
	ctor invokes, "main" does not 

	when called :
		$ ./nomain 
	ctor invokes, "main" invokes
	
*/
__attribute__((constructor)) static void myctor(int argc, char **argv, char* envp[]){

        printf("in ctor\n");
}
int _(void);
void _start() 
{ 
    int x = _(); //calling custom main function 
    exit(x); 
} 
int _() {
    printf("in main: %s\n", __FUNCTION__);
    while (1) ;
    return 0;
}
