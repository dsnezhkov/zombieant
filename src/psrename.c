#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
  

#define MAX_MESG 128
// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[MAX_MESG]; 
} message; 
  
int main(int argc, char *argv[]) 
{ 

	 if (argc != 3){
		 fprintf(stderr, "Usage: %s <key> <name[MAX_MESG:%d]>\n", argv[0], MAX_MESG);
		 exit(1);	
	 }

    int seed = atoi(argv[1]); 
    key_t key = ftok(".", seed);
    int msgid = msgget(key, 0666 | IPC_CREAT);
  
    message.mesg_type = 1; 

	 strncpy(message.mesg_text, argv[2], MAX_MESG);
    // display the message 
    printf("Sending to MSGID %d: %s \n", msgid, message.mesg_text); 
  
    // msgsnd to send message 
    msgsnd(msgid, &message, sizeof(message), 0); 
  
    return 0; 
}
