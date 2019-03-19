#include <stdio.h>
#include "weakref_main.h"



int main(void)
{
    if (debug) {
        printf("main: debug() CALLED\n");
		debug();
    }
    else {
        printf("main: debug() NOT AVAILABLE \n");
    }

    fflush(stdout);
    return 0;
}
