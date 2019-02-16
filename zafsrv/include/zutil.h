#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

#include "log.h"
#include "zmemfd.h"
#include "zstructs.h"


extern char  *logFile;
extern FILE  *fp; // Log descriptor

int   setKernel(void);
void  gen_random(char *s, const int len);
void  cleanup(int cleanLog);
