#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// gcc -o main main.c -L/root/Code/psevade -l:libctx.so.1
extern void die(const char*, ...);

int main(int argc, char **argv) {

	int c=0;
    while (1) { 
			printf("trying : %d\n", c);
			if (c == 5){
				die("it's over at %d\n", c);
			}
			c++;
			sleep(1) ; 
	}
    return 0;
}
