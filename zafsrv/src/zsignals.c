#include "zsignals.h"

volatile sig_atomic_t sigflag  = 0; 

void setSignalHandlers(){
    signal(SIGHUP,  handleSig);
    signal(SIGUSR1, handleSig);
    signal(SIGUSR2, handleSig);
    signal(SIGINT,  handleSig);
    signal(SIGTERM, handleSig);
}

void handleSig(int signo) {
    /*
     * Signal safety: It is not safe to call clock(), printf(),
     * or exit() inside a signal handler. Instead, we set a flag.
     */
    switch(signo){
        case SIGHUP:
            log_warn("Signal handler got SIGHUP signal ... ");
            sigflag=1;
            doSigHup();
            break;
        case SIGUSR1:
            log_warn("Signal handler got SIGUSR1 signal ... ");
            sigflag=2;
            doSigUsr1();
            break;
        case SIGUSR2:
            log_warn("Signal handler got SIGUSR1 signal ... ");
            sigflag=3;
            doSigUsr2();
            break;
        case SIGINT:
            log_warn("Signal handler got SIGINT signal ... ");
            sigflag=4;
            doSigInt();
            break;
        case SIGTERM:
            log_warn("Signal handler got SIGTERM signal ... ");
            sigflag=5;
            doSigTerm();
            break;
    }
}

void doSigHup(void){
    sigflag = 1;
    log_debug("Executing SIGHUP code here ...");
}
void doSigUsr1(void){
    sigflag = 2; // check or reset
    log_debug("== Stats: \tPID: %d, Log: %s ==", getpid(), logFile);
    log_debug("Executing SIGUSR1 code here ...");
}
void doSigUsr2(void){
    sigflag = 3; // check or reset
    log_debug("Executing SIGUSR2 code here ...");
    log_set_level(1); // increase logging to DEBUG
}
void doSigInt(void){
    sigflag = 4; // check or reset
    log_debug("Executing SIGINT code here ...");
    log_debug("=== ZAF down ===");
    cleanup(0); // leave log intact
    exit(0);
}
void doSigTerm(void){
    sigflag = 5; // check or reset
    log_debug("Executing SIGTERM code here ...");
    log_debug("=== ZAF down ===");
    // - logrotate log
    // - Send log offsite
    cleanup(1); // remove log
    exit(0);
}

