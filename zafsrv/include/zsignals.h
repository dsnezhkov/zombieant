#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>


#include "log.h"

void setSignalHandlers(void);
void handleSig(int signo);
void doSigHup(void);
void doSigUsr1(void);
void doSigUsr2(void);
void doSigInt(void);
void doSigTerm(void);

extern void cleanup(int removelog);
extern char *logFile;
