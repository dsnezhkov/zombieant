#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>


#include "weakref.h"

static int gmon_ct=0, cxa_ct=0;

void debug(){
    printf("debug()\n");
    fflush(stdout);

    if (mstat) {
        printf("debug: stat()\n");
        mstat();
    }
    else {
        printf("debug: ---\n");
    }

    fflush(stdout);
}

void __gmon_start__(){
	 if (gmon_ct < 1 ){
		 printf("in __gmon_start__ weak hook\n");
		 gmon_ct++;

	 }else{
		 printf("in __gmon_start__ weak hook but already started\n");
	 }
}

void __cxa_finalize() {
	 if (cxa_ct < 1 ){
		 printf("in __cxa_finalize weak hook\n");
		 cxa_ct++;

	 }else{
		 printf("in __cxa_finalize weak hook but already started\n");
	 }
}
