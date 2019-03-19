#include <stdio.h>            // printf 
#include <string.h>           // strerror
#include <stdlib.h>           // exit
#include <errno.h>            // errno
#include <unistd.h>           // sleep
#include <sys/personality.h>  // personality
#include <sys/wait.h> 
#include <stdbool.h>
#include <sys/ptrace.h>
#include <sys/user.h>         // user_regs_struct
#include <execinfo.h>

#define BT_BUF_SIZE 100


static void before_main(void) __attribute__((constructor ));
static void before_main(void)
{
    errno = 0;

    int m_personality_orig;
    int pid;
    //bool m_personality_set = false;
    char *argv[] = { "/bin/ls", "/tmp", 0 };
    extern char** environ;
    printf("Before main()\n");
   
    unsetenv("LD_PRELOAD"); // Do not preload forked chid, just weaken it
    // create a child process
    printf("Before fork: PPID: %d PID: %d\n", getppid(), getpid());
    pid = fork();
    printf("After fork: PPID: %d PID: %d\n", getppid(), getpid());
  
    if (0 > pid) {
        printf("Error during forking: %s\n", strerror(errno));
        exit(1);
    }
  
    // child process
    if (0 == pid) {
        printf("In child: PPID: %d PID: %d\n", getppid(), getpid());
        m_personality_orig = personality (0xffffffff);
        if (errno == 0 && !(m_personality_orig & ADDR_NO_RANDOMIZE)) {
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            personality (m_personality_orig | ADDR_NO_RANDOMIZE);
            if (errno == 0) {
                printf("Unrandomized. PID: %d\n", getpid());
                for (char **env = environ; *env != 0; env++)
                {
                    char *pair = *env;
                    printf("%p %s\n", pair, pair);
                }
                printf("About to execve()\n");
                /* BACKTRACE */
			   int j, nptrs;
			   void *buffer[BT_BUF_SIZE];
			   char **strings;

			   nptrs = backtrace(buffer, BT_BUF_SIZE);
			   printf("backtrace() returned %d addresses\n", nptrs);

			   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
				  would produce similar output to the following: */

			   strings = backtrace_symbols(buffer, nptrs);
			   if (strings == NULL) {
				   perror("backtrace_symbols");
				   exit(EXIT_FAILURE);
			   }

			   for (j = 0; j < nptrs; j++)
				   printf("%s\n", strings[j]);

			   free(strings);

                /* END BACKTRACE */
                execve(argv[0], argv, environ);
                printf("After execve()\n");
            }else{
                printf("Error disabling address space randomization: %s\n", strerror(errno));
                exit(1);
            }
        }else{
            printf("Error saving personality: %s\n", strerror(errno));
            exit(1);
    }
  }

  // parent process
  printf("In Parent: PPID: %d PID: %d\n", getppid(), getpid());

  int status;
  struct user_regs_struct regs;

  wait(&status);
  while(1407 == status) {
    // get registers
    if (0 != ptrace(PTRACE_GETREGS, pid, 0, &regs))
      printf("Error during ptrace: %s\n", strerror(errno));      
    
    // if the syscall is close(for example)
    // wait so we can get memory maps.
    if (3 == regs.orig_rax) {
      // This scanf is for waiting for input,
      // while we cat /proc/pid/maps
      // to see if it differs from 
      // previous /proc/pid/maps
      scanf("1"); 
    }

    // lets the child to continue his work until next sys call or sys exit
    if (0 != ptrace(PTRACE_SYSCALL, pid, 0, 0) != 0)
      printf("Error during ptrace: %s\n", strerror(errno));

    wait(&status);
  }
}

static void after_main(void) __attribute__((destructor ));
static void after_main(void){
    printf("After main()\n");
}

