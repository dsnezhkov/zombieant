#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>



/*
Load shared library and invoke entry point
Optionally, pass parameters to the shared library
*/
int main(int argc, char** argv) {
       

    if (argc < 2){
      fprintf(stderr, "Usage: %s <module.so> [args]\n", argv[0]);
      exit(1);
    }


   int i, v = 0, size = argc - 1;
   void *handle;
   void (*Entry)(char*);

   char *str = (char *)malloc(v);
   for(i = 2; i <= size; i++) {
        str = (char *)realloc(str, (v + strlen(argv[i])));
        strcat(str, argv[i]);
        strcat(str, " ");
    }

    handle = dlopen(argv[1], RTLD_LAZY);
	*(void**)(&Entry) = dlsym(handle, "Entry");
	Entry(str);

	exit(EXIT_SUCCESS);
}
