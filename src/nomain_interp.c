#include <stdio.h> 
#include <stdlib.h> 

// gcc -shared -Wl,-e,fn_no_main nomain.c -o nomain.so
// 32bit const char my_interp[] __attribute__((section(".interp"))) = "/lib/ld-linux.so.2";
// patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2
// const char my_interp[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";
const char my_interp[] __attribute__((section(".interp"))) = "/usr/local/bin/gelfload-ld-x86_64";
int fn_no_main() {
	printf("This is a function without main\n");
	exit(0);
}
