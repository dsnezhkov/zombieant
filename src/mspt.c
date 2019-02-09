#include "msptso.h"

volatile sig_atomic_t gotint = 0;

// Constructor called by ld.so before the main. This is not guaranteed but works.
__attribute__((constructor)) static void mctor(int argc, char **argv, char** envp)
{

    printf("Before main()\n");

    argv = save_ps_display_args(argc, argv);
    init_ps_display("hello, world");
    //try_division(1,0);
    setSigHandler(500000);

}

void doWork(void){
	printf("Executing payloads here ...\n");
    set_ps_display("new title!", true);
}

void setSigHandler(int sleep_interval){

	printf("Setting SIGHUP Signal handler ...\n");
    printf("PPID: %d PID: %d \n", getppid(), getpid());
    signal(SIGHUP, handleSigHup);
    for (;;) {
        if (gotint)
            break;
        usleep(sleep_interval);
    }
	printf("Signal handler got signal ... \n");
	doWork();
}
 
void handleSigHup(int signal) {
    /*
     * Signal safety: It is not safe to call clock(), printf(),
     * or exit() inside a signal handler. Instead, we set a flag.
     */
    gotint = 1;
}
 

// ref: http://rosettacode.org/wiki/Detect_division_by_zero#C 
/*
 * This SIGFPE handler jumps to fpe_env.
 *
 * A SIGFPE handler must not return, because the program might retry
 * the division, which might cause an infinite loop. The only safe
 * options are to _exit() the program or to siglongjmp() out.
 */
void fpe_handler(int signal, siginfo_t *w, void *a)
{
	printf("In SIGFPE handler\n");
	siglongjmp(fpe_env, w->si_code);
	/* NOTREACHED */
}
 
/*
 * Try to do x / y, but catch attempts to divide by zero.
 */
void try_division(int x, int y)
{
	struct sigaction act, old;
	int code;
	/*
	 * The result must be volatile, else C compiler might delay
	 * division until after sigaction() restores old handler.
	 */
	volatile int result;
 
	/*
	 * Save fpe_env so that fpe_handler() can jump back here.
	 * sigsetjmp() returns zero.
	 */
	code = sigsetjmp(fpe_env, 1);
	if (code == 0) {
		/* Install fpe_handler() to trap SIGFPE. */
		act.sa_sigaction = fpe_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		if (sigaction(SIGFPE, &act, &old) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		/* Do division. */
		result = x / y;
 
		/*
		 * Restore old hander, so that SIGFPE cannot jump out
		 * of a call to printf(), which might cause trouble.
		 */
		if (sigaction(SIGFPE, &old, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		printf("%d / %d is %d\n", x, y, result);
	} else {
		/*
		 * We caught SIGFPE. Our fpe_handler() jumped to our
		 * sigsetjmp() and passes a nonzero code.
		 *
		 * But first, restore old handler.
		 */
		if (sigaction(SIGFPE, &old, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		/* FPE_FLTDIV should never happen with integers. */
		switch (code) {
		case FPE_INTDIV: /* integer division by zero */
		case FPE_FLTDIV: /* float division by zero */
			printf("%d / %d: caught division by zero!\n", x, y);
			doWork();
			break;
		default:
			printf("%d / %d: caught mysterious error!\n", x, y);
			break;
		}
	}
}
