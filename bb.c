#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <utmp.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <linux/fs.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 

#include "bb.h"


// Save argc/argv. Set name of process

char **largv;
int largc;
char * name = "smtp: accepting connections";
int msgid; 

void sig_handler(int signo)
{
  if (signo == SIGUSR1){
#ifdef  BB_DEBUG
    printf("received SIGUSR1\n");
#endif 
    // msgrcv to receive message 
    //msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT); 
    msgrcv(msgid, &message, sizeof(message), 1, 0); 
  
    // display the message 
#ifdef  BB_DEBUG
    printf("Data Received is : %s \n",  
                    message.mesg_text); 
#endif 

    printf("sending '%s'to setNewName\n",  
                    message.mesg_text); 
    // attempt to set new name if and when works 
    // should be generic enough across linux distros.
    //setNewName(largc, largv, name);
    setNewName(largc, largv, message.mesg_text); // check for size
  }
}


/* rename process in ps and proc tables */
void setNewName(int argc, char *argv[], char* procname){


	 printf("setNewName before step 1\n");
    // Step 1: set proc name
    // By itself this change does not reflect in PS table (from /proc/pid/cmd)
    // only in /proc/pid/comm
	 // FYI: procname is up to 16 bytes.
    if (prctl(PR_SET_NAME, (unsigned long) procname, 0, 0, 0) != 0) {

//#ifdef  BB_DEBUG
        fprintf(stderr, "Got error '%s' when setting process name with prctl() to '%s'\n", strerror(errno), procname);
//#endif
		  ;
    } 
	 printf("setNewName after prctl\n");

   // Step 2: hijack argv[]
   // ref: https://stackoverflow.com/questions/4461289/change-thread-name-on-linux-htop
   // Then let's directly modify the arguments
   // This needs a pointer to the original arvg, as passed to main(),
   // and is limited to the length of the original argv[0]
	 printf("argv[0]: %s\n", argv[0]);
   size_t argv0_len = strlen(argv[0]);
	 printf("strlen argv[0] %d\n", argv0_len);
   size_t procname_len = strlen(procname);
	 printf("procname_len %d\n", procname_len);
   size_t max_procname_len = (argv0_len > procname_len) ? (procname_len) : (argv0_len);
	 printf("max_procname_len %d\n", max_procname_len);

	printf("setNewName before strncopy\n");
   // Copy the maximum size of a name
   strncpy(argv[0], procname, max_procname_len);
   // Clear out the rest (yes, this is needed, or the remaining part of the old
   // process name will still show up in ps)
   memset(&argv[0][max_procname_len], '\0', argv0_len - max_procname_len);
	 printf("setNewName after argv[0]  memset\n");

   // Clear the other passed arguments, optional
   // Needs to know argv and argc as passed to main()
   /* for (size_t i = 1; i < largc; i++) {
	  printf("setNewName memset argv[i]");
     memset(argv[i], '\0', strlen(argv[i]));
   }*/

}

void doWork(void)
{
  printf("Working ...\n");
  while(1) 
    sleep(BB_WAIT); /* wait X seconds */
}

void backgroundDaemonLeader(void){
	pid_t pid,sid;

	/* Fork off the parent process */
	pid = fork(); if (pid < 0) { exit(EXIT_FAILURE); }
	/* Exit the parent process. */
	if (pid > 0) { exit(EXIT_SUCCESS); }

	/* Create a new SID for the child process */
	sid = setsid(); if (sid < 0) { exit(EXIT_FAILURE); }

	/* Close out the standard file descriptors */
#ifndef BB_DEBUG
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
#endif     
}

void hookSignal(int signo){
	// Set Signal handler for name change
   if (signal(SIGUSR1, sig_handler) == SIG_ERR){
#ifdef BB_DEBUG
  		printf("\ncan't catch SIGINT\n");
#endif     
	 ;
	}
}

void hookNameSource(void){
	key_t key; 
  
   // ftok to generate unique key 
   key = ftok("/", 65); 
   printf("KEY: %d\n", key);
  
   // msgget creates a message queue 
   // and returns identifier 
   msgid = msgget(key, 0666 | IPC_CREAT); 
   printf("MSGID: %d\n", msgid);
  
  
}
/*------------------------------------*/
int main(int argc, char** argv) {
       

	// Save args for processing. They will get overwritten
	largv = argv;
	largc = argc;
 
	// Daemonize
   backgroundDaemonLeader();

	// Rename on command
   hookSignal(SIGUSR1);

	hookNameSource();

	/* Work! */
   doWork();

	exit(EXIT_SUCCESS);
}
