// #define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>

//#include <dlfcn.h>


// ------------------------------------------------------------------------- //
/*

# PSEvade SO
Side loading of pre-main functionality to hijack signals 
and rename process we have control over in PS table. 
This is done to evade detection by tools relying solely on the PS table info.
This is not a userland rootkit. 


# EDR guidance:

changes values in:
	/proc/pid/cmdline
	/proc/pid/comm

	Affects PS table utilities:
	(e.g. top, ps)

   Resets LD_PRELOAD in /proc/pid/environ 

intact values in:
	leaves preloaded .so in memory maps
		/proc/pid/maps

	Does not affect others:
	(e.g. lsof -p pid ) 

*/


// ------------------------------------------------------------------------- //
/* 
	We have to grab and save argv/argc pointers globally
	so they are visible for update
	Provision for shim commands
*/

char ** largv = NULL;
int     largc;
char ** lenvp = NULL;

#define MAX_MESG 128

struct comm {
	int cmd_id;
	 union cmd_t {
		int  msgq_no;
		char cmd_args[MAX_MESG];
	 } m;
} commands;

// structure and indetifiers for message queue 
struct mesg_buffer {
	 long mesg_type;
	 char mesg_text[MAX_MESG];
} message;

int msgid;
key_t key;


void setRandezvous(void){

   //ftok to generate unique key 
	
   key = ftok(".", commands.m.msgq_no);

   // msgget wants a message queue with info, 
   // create if not present,
   // return identifier 
   msgid = msgget(key, 0666 | IPC_CREAT);
   if (msgid < 0){
		fprintf(stderr, "%d: %s\n", errno, strerror(errno));
   }else{
   	printf("pid:%d, key:0x%x, msqid:%d\n", getpid(), key, msgid);
	}
}


void SetProcessName(void){

   char* procname = message.mesg_text;

	switch (commands.cmd_id){
		case 1:
			printf("immediate static\n");
   		procname = commands.m.cmd_args; 
			break;
		case 2:
			printf("delayed static\n");
   		procname = commands.m.cmd_args; 
			break;
		case 3:
			printf("delayed dynamic\n");
			// msgrcv to receive message 
			msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);

			// to destroy the message queue  (allow one rename)
			msgctl(msgid, IPC_RMID, NULL); 
			printf("message q destroyed: %d\n", msgid); 
			break;
		default:
			printf("invalid command\n");
	}


   /* 
		Step 1: Set the name of the calling thread via prctl()
	 		visible in /proc/self/task/[tid]/comm, where tid is the name of the calling thread.
	*/

#ifdef PSDEBUG
	 printf("Setting procname: %s\n", procname);
#endif 
    if (prctl(PR_SET_NAME, (unsigned long) procname, 0, 0, 0) != 0) {
#ifdef PSDEBUG
        fprintf(stderr, "Got error '%s' when setting process name with prctl() to '%s'\n", strerror(errno), procname);
#endif 
		;
    }


    /* 
		 Step 2: Hijack argv[0] (and nullify the rest of args)
	  		WARNING : By that time hopefully the callee process has consumed the arguments
	 */
	
	
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

// Signal handler
static void sig_handler(int sig) 
{ 

#ifdef PSDEBUG
	 printf("Caught signal %d\n", sig); 

    for (int i=0; i<largc; i++) {
        printf("%s: largv[%d] = '%s'\n", __FUNCTION__, i, largv[i]);
    }
#endif

	 // Rename process
	 SetProcessName();
}

void cleanEnv(void){

	 // a. remove from /proc/pid/environ
	 while (*lenvp) {
#ifdef PSDEBUG
		//printf("%s\n", *lenvp);
#endif
      if (strncmp(*lenvp, "LD_PRELOAD=", 11) == 0)
        memset(*lenvp, 0, strlen(*lenvp));
      if (strncmp(*lenvp, "LD_CMD=", 7) == 0)
        memset(*lenvp, 0, strlen(*lenvp));
      if (strncmp(*lenvp, "LD_BG=", 6) == 0)
        memset(*lenvp, 0, strlen(*lenvp));

      lenvp++;
    }

	 // b. remove from process memory if present to be sure
	 unsetenv("LD_PRELOAD"); 
	 unsetenv("LD_CMD"); 
	 unsetenv("LD_BG"); 
}

void die(const char* format, ...){

   va_list vargs;
   va_start (vargs, format);
   vfprintf (stderr, format, vargs);
   fprintf (stderr, ".\n");
   va_end (vargs);
	exit(EXIT_FAILURE);
}

int backgroundDaemonLeader(char * cmd_bg){

    // Fork, allowing the parent process to terminate.
    pid_t pid = fork();
    if (pid == -1) {
        die("failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        _exit(0);
    }

    // Start a new session for the daemon.
    if (setsid()==-1) {
        die("failed to become a session leader while daemonising(errno=%d)",errno);
    }

    // Fork again, allowing the parent process to terminate.
    signal(SIGHUP,SIG_IGN);
    pid=fork();
    if (pid == -1) {
        die("failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        _exit(0);
    }

    // Set the current working directory to the root directory.
    if (chdir("/tmp") == -1) {
        die("failed to change working directory while daemonising (errno=%d)",errno);
    }

    // Set the user file creation mask to zero.
    umask(0);


	 // Close then reopen standard file descriptors. Depending if you are drving a binary that
	 // depends on fd's remain open or not you may need to make decisions. 
	 // Example: if you are emulating /usr/bin/cat, the emulated binary needs to keep fd's open
	 // by definition. Closing them may terminate it.
	 if ( strncasecmp(cmd_bg, "fdclose", 7) == 0 ) {

		 close(STDIN_FILENO);
		 close(STDOUT_FILENO);
		 close(STDERR_FILENO);
		 if (open("/dev/null",O_RDONLY) == -1) {
			  die("failed to reopen stdin while daemonising (errno=%d)",errno);
		 }
		 if (open("/dev/null",O_WRONLY) == -1) {
			  die("failed to reopen stdout while daemonising (errno=%d)",errno);
		 }
		 if (open("/dev/null",O_RDWR) == -1) {
			  die("failed to reopen stderr while daemonising (errno=%d)",errno);
		 }
	 }
   return 0;
}


int processCommands(void){

	char *cmd=NULL, *key=NULL, *val=NULL;
	int cmd_len, cmd_len_limit=128;


   printf("%s: enter\n", __FUNCTION__);
	if ( (cmd = (char*) getenv("LD_CMD")) == NULL ){
		printf("Invalid LD_CMD");
	   return 0;

	}else{
		printf("LD_CMD: %s\n", cmd);

		cmd = strdup(cmd); 
		key = strtok(cmd, ":");
		val = strtok(NULL, ":");
		printf("%s => %s\n", key, val);

		cmd_len = strlen(val);
		if (cmd_len > (cmd_len_limit))
			cmd_len = cmd_len_limit;

		// immediately
		if ( strncmp(key, "r", 1) == 0 ){

				printf("immediate rename to static(%s)\n", val);
				commands.cmd_id=1;	
				strncpy(commands.m.cmd_args, val, cmd_len) ;	
				printf("static name (%s)\n", val);
		      signal(SIGUSR1, sig_handler); 
				raise(SIGUSR1);

		// on signal
		} else if ( strncmp(key, "R", 1) == 0 ){
				printf("delayed rename to static(%s)\n", val);
				commands.cmd_id=2;	
				strncpy(commands.m.cmd_args, val, cmd_len) ;	
				printf("static name (%s)\n", val);
		      // evade: wait for external command
		      signal(SIGUSR1, sig_handler); 

		// on signal and mesgq
		} else if ( strncmp(key, "M", 1) == 0 ){
				commands.cmd_id=3;	
				commands.m.msgq_no = atoi(val) ;	
				printf("delayed rename to dynamic (method: %s)\n", val);
				// evade: establish control channel
				setRandezvous();
		      // evade: wait for external command
		      signal(SIGUSR1, sig_handler); 

		} else {
				printf("error: unsupported command: %s\n", key);
		}
	}

	return 1;
}

// Constructor called by ld.so before the main. This is not guaranteed but works.
__attribute__((constructor)) static void myctor(int argc, char **argv, char** envp)
{

	 // Save pointers to argv/argc/envp
	 largv=argv;
	 largc=argc;
	 lenvp=envp;

#ifdef PSDEBUG
	 printf("pointer argv: %p\n", argv);
	 printf("pointer largv: %p\n", largv);
    for (int i=0; i<argc; i++) {
        printf("%s: argv[%d] = '%s'\n", __FUNCTION__, i, argv[i]);
    }
#endif

	

	processCommands();

	char * bg_cmd=NULL;
	if ( (bg_cmd = (char*) getenv("LD_BG")) != NULL )
	 	backgroundDaemonLeader(bg_cmd);

   // always evade: clean LD_PRELOAD pointers, LD_CMD
	cleanEnv();

}

// Destructor if needed.
__attribute__((destructor)) static void mydtor(void) { }
