/*
 * Hook main() using LD_PRELOAD, because why not?
 * Obviously, this code is not portable. Use at your own risk.
 *
 * Compile using 'gcc hax.c -o hax.so -fPIC -shared -ldl'
 * Then run your program as 'LD_PRELOAD=$PWD/hax.so ./a.out'
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>

/* Trampoline for the real main() */
static int (*main_orig)(int, char **, char **);

/* Our fake main() that gets called by __libc_start_main() */
int main_hook(int argc, char **argv, char **envp)
{
    for (int i = 0; i < argc; ++i) {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    printf(">__libc_start: main_hook\n");
    int ret = main_orig(argc, argv, envp);
    printf("< hook exited .. \n");
    return ret;
}

/*
 * Wrapper for __libc_start_main() that replaces the real main
 * function with our hooked version.
 */
int __libc_start_main(
    int (*main)(int, char **, char **),
    int argc,
    char **argv,
    int (*init)(int, char **, char **),
    void (*fini)(void),
    void (*rtld_fini)(void),
    void *stack_end)
{
    /* Save the real main function address */
    main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with our custom main function */
    return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}
