#include "mctrso.h"


void before_main(void) __attribute__((constructor (101)));
void before_main(void)
{

    /* This is run before main() */
    printf("Before main()\n");

}

//delayed invocation
void after_main(void) __attribute__((destructor (101)));
void after_main(void)
{

    /* This is run after main() returns (or exit() is called) */
    printf("After main()\n");

}

