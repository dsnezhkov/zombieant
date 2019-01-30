#include <stdio.h>
#include "weakref.h"

/*extern void (*debugfunc)(void) = debug;

if (debugfunc) (*debugfunc)();
*/



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
