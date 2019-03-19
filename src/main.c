#include <stdio.h>
#include <unistd.h>


int main(int argc, char **argv) {

    int i;

    printf("%s: ", __FUNCTION__);
    for (i=1; i < 4; i++) { 
        printf(" %d ", i);
        fflush(stdout);
		usleep(500000) ; 
 	}
    printf("\n");
    return 0;
}
