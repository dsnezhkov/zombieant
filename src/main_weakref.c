#include <stdio.h>
#include "weakref_main.h"



int main(void)
{
    if (debug) {
        printf("main: debug()\n");
		  debug();
    }
    else {
        printf("main: ---\n");
    }

    if (mstat) {
        printf("main: test trampoline to stat()\n");
		mstat();
    }

    fflush(stdout);
    return 0;
}
