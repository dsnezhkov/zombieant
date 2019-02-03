#include <stdio.h>
#include <stdlib.h>

int _(void);
void __data_frame_e() 
{ 
    int x = _(); //calling custom main function 
    exit(x); 
} 
int _() {
    printf("in main: %s\n", __FUNCTION__);
    while (1) ;
    return 0;
}
