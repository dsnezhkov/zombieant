#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>

void fpe_handler(int signal, siginfo_t *w, void *a);
void try_division(int x, int y);

static sigjmp_buf fpe_env;
