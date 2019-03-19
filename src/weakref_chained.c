#include "weakref_so.h"

void debug(){
    printf("Weakref Lib1: debug()\n");
    fflush(stdout);

    if (mstat) {
        printf("Weakref Lib1: debug: calling chained mstat()\n");
        mstat();
    }
    else {
        printf("debug: mstat() NOT AVAILABLE\n");
    }


    fflush(stdout);
}
