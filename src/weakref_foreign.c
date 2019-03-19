#include "weakref_so.h"

static int gmon_ct=0, cxa_ct=0;


// Injection to any ELF at start
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
