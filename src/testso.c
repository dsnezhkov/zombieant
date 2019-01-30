#include <stdio.h>

//gcc -Wall -Wextra -fPIC -shared testso.c -Wl,-soname,libtestso.so -o libtestso.so


void before_main(void) __attribute__((constructor (101)));
void before_main(void)
{

    /* This is run before main() */
    printf("Before main()\n");

}

//delayed invocation
static void after_main(void) __attribute__((destructor (101)));
static void after_main(void)
{

    /* This is run after main() returns (or exit() is called) */
    printf("After main()\n");

}
