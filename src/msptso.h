#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include "prename/spt_status.h"


void doWork(void);
void setSigHandler(int sleep_interval);
void handleSigHup(int signal);

void fpe_handler(int signal, siginfo_t *w, void *a);
void try_division(int x, int y);

static sigjmp_buf fpe_env;
