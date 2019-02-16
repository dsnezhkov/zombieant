#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"


#define DAEMON_CHDIR "/tmp"                // daemon directory to operate from 


extern void  setSignalHandlers(void);
extern void  doWork(void);
extern char  *logFile;

int backgroundDaemonLeader(void);

