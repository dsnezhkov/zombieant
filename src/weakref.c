#include "weakref_so.h"

static int gmon_ct=0, cxa_ct=0;

// Weak ref check example. Alternative to dlopen()
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

// Injection to any ELF
void __gmon_start__(){
	 if (gmon_ct < 1 ){
	   printf("in __gmon_start__ weak hook\n");
	   gmon_ct++;

	 }else{
	   printf("in __gmon_start__ weak hook but already started\n");
	 }


}

// Evasion trampoline (atexit hook)
void __cxa_finalize() {
	 if (cxa_ct < 1 ){
		 printf("in __cxa_finalize weak hook\n");
		 cxa_ct++;

	 }else{
		 printf("in __cxa_finalize weak hook but already started\n");
	 }
}

// Java Jni
void _Jv_RegisterClasses() {
		 printf("in _Jv_RegisterClasses weak hook\n");
}


