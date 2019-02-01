#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

void debug() __attribute__((weak));
void mstat() __attribute__((weak));
void __gmon_start__() __attribute__((weak));
void __cxa_finalize() __attribute__((weak));
void _Jv_RegisterClasses() __attribute__((weak));
