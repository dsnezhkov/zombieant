#define _GNU_SOURCE
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
#include <dlfcn.h>

#include "bb.h"


// gcc -o bb bb.c  -ldl

// Save argc/argv. Set name of process
char **largv;
int largc;

void sig_handler(int signo)
{
  if (signo == SIGUSR1){
#ifdef  BB_DEBUG
    printf("received SIGUSR1\n");
#endif 
    setNewName(); // dispatch minimal info from handler
  }
}


/* 
	get name from an IPC, 
	rename process in ps and proc tables 
*/
int setNewName(void){

	key_t key; 
	int msgid;

	// structure for message queue 
	struct mesg_buffer { 
		 long mesg_type; 
		 char mesg_text[100]; 
	} message; 

   // ftok to generate unique key 
   key = ftok("/", 65); 
   printf("KEY: %d\n", key);
  
   // msgget wants a message queue with info, 
	// create if not present,
   // return identifier 
   msgid = msgget(key, 0666 | IPC_CREAT); 
   printf("MSGID: %d\n", msgid);

    // msgrcv to receive message 
   msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT); 

	char* procname = message.mesg_text;

	// to destroy the message queue 
   // msgctl(msgid, IPC_RMID, NULL); 
   // printf("message q destroyed: %d\n", msgid); 

	printf("Setting procname: %s\n", procname);
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


   // Step 2: hijack argv[]
   // ref: https://stackoverflow.com/questions/4461289/change-thread-name-on-linux-htop
   // Then let's directly modify the arguments
   // This needs a pointer to the original arvg, as passed to main(),
   // and is limited to the length of the original argv[0]
   size_t argv0_len = strlen(largv[0]);
   size_t procname_len = strlen(procname);
   size_t max_procname_len = (argv0_len > procname_len) ? (procname_len) : (argv0_len);

   // Copy the maximum size of a name
   strncpy(largv[0], procname, max_procname_len);
   // Clear out the rest (yes, this is needed, or the remaining part of the old
   // process name will still show up in ps)
   memset(&largv[0][max_procname_len], '\0', argv0_len - max_procname_len);

   // Clear the other passed arguments, optional
   // Needs to know argv and argc as passed to main()
    for (size_t i = 1; i < largc; i++) {
     memset(largv[i], '\0', strlen(largv[i]));
   }


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
   if (signal(signo, sig_handler) == SIG_ERR){
#ifdef BB_DEBUG
  		printf("\ncan't catch signal\n");
#endif     
	 ;
	}
}

  
  
/*------------------------------------*/
int main(int argc, char** argv) {
       

	 if (argc != 2){
       fprintf(stderr, "Usage: %s <module.so>\n", argv[0]);
       exit(1);
    }



	// Save args for processing. They will get overwritten
	largv = argv;
	largc = argc;
 
	// Daemonize
   backgroundDaemonLeader();

	// Rename on command
   hookSignal(SIGUSR1);


	void *handle;
   void (*entry)(const char*);

   handle = dlopen(argv[1], RTLD_LAZY);
	*(void**)(&entry) = dlsym(handle, "entry");
	entry("localhost");

	/* Work! */
   doWork();

	exit(EXIT_SUCCESS);
}
