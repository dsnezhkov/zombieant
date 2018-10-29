#include<stdlib.h>

void setNewName(int argc, char *argv[], char* procname);


// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[100]; 
} message; 



#ifndef BB_WAIT
#define BB_WAIT 10 // Wait interval between pranks
#endif 

#ifndef BB_DEBUG
#define BB_DEBUG 
#endif 

