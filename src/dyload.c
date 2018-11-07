#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>


#ifndef BB_WAIT
#define BB_WAIT 10 // Wait interval between pranks
#endif 

#ifndef BB_DEBUG
#define BB_DEBUG 
#endif

// gcc -o bb bb.c  -ldl

void doWork(void)
{
  printf("Working ...\n");
  while(1) 
    sleep(BB_WAIT); /* wait X seconds */
}

/*------------------------------------*/
int main(int argc, char** argv) {
       

	if (argc != 2){
      fprintf(stderr, "Usage: %s <module.so>\n", argv[0]);
      exit(1);
   }

	void *handle;
   void (*entry)(const char*);

   handle = dlopen(argv[1], RTLD_LAZY);
	*(void**)(&entry) = dlsym(handle, "entry");
	entry("localhost");

	/* Work! */
   doWork();

	exit(EXIT_SUCCESS);
}
