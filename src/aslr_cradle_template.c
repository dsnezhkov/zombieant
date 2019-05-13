#include <stdio.h>            
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>          

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>


#define DAEMON_CHDIR "/tmp"

int daemonize(char *buf);

//gcc -z execstack -o template template.c
void main() { 
    char *key = "secret"; // sample data to read
    char *buf = (char*)calloc(400, sizeof(char)); // 400 codecave sample
    char *bufc = (char*)calloc(1, sizeof(char)); 

    printf("pid: %d\n", getpid()); 
    printf("key: %p\n", key); 
    printf("buf: %p\n", buf); 

    printf("Sleeping letting parent catch up\n");
    sleep(1);

#ifndef T_DEBUG
    int result = 0; 
    while ( (result = memcmp(buf, bufc, 1)) == 0 ){
        printf("Sleeping waiting for injection\n");
        sleep(3);
    }
    printf("Injection requested!\n");
#endif
    daemonize(buf);
}

int daemonize(char *buf){

    // Fork, allowing the parent process to terminate.
    pid_t pid = fork();
    if (pid == -1) {
        // after first fork(), unsuccessful. exiting with error.
        _exit(1);
    } else if (pid != 0) {
        // after first fork() in parent. exiting by choice.
        _exit(0);
    }

    // Start a new session for the daemon.
    if (setsid()==-1) {
         // failed to become a session leader while daemonising
         ;
    }

    // Fork again, allowing the parent process to terminate.
    signal(SIGCHLD, SIG_IGN); // avoid defunct processes. 
    pid=fork();
    if (pid == -1) {
        // after second fork(), unsuccessful. exiting with error.
        _exit(1);
    } else if (pid != 0) {
        // after second fork() in parent. exiting by choice.
        _exit(0);
    }

    // Set the current working directory to the root directory.
    chdir(DAEMON_CHDIR); // do nothing if it cannot do so, or maybe attempt to chdir elsewhere

    // Set the user file creation mask to zero.
    umask(0);

    /*
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Reopen STDIN/OUT/ERR to Null
    if (open("/dev/null",O_RDONLY) == -1) {
      log_error("Daemon: failed to reopen stdin while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_WRONLY) == -1) {
      log_error("Daemon: failed to reopen stdout while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_RDWR) == -1) {
         log_error("Daemon: failed to reopen stderr while daemonising (errno=%d)",errno);
    }
    */

#ifndef T_DEBUG
    ((void (*) (void)) buf) ();
#else
    sleep(10000000);
#endif
    return 0;
}
