#include "zservice.h"

int backgroundDaemonLeader(void){

    // Fork, allowing the parent process to terminate.
    log_debug("Daemon: before first fork()");
    pid_t pid = fork();
    log_debug("Daemon: after first fork()");
    if (pid == -1) {
        log_error("Daemon: failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        log_debug("Daemon: after first fork(0 in parent. exiting.");
        _exit(0);
    }

    // Start a new session for the daemon.
    log_debug("Daemon: before setsid()");
    if (setsid()==-1) {
        log_error("Daemon: failed to become a session leader while daemonising(errno=%d)",errno);
    }

    // Fork again, allowing the parent process to terminate.
    //signal(SIGHUP,SIG_IGN);
    log_trace("Daemon: setting up SIGCHLD");
    signal(SIGCHLD, SIG_IGN); // avoid defunct processes. 
    log_trace("Daemon: before second fork()");
    pid=fork();
    log_trace("Daemon: after second fork()");
    if (pid == -1) {
        log_error("Daemon: failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        log_trace("Daemon: after first fork() in parent. exiting.");
        _exit(0);
    }

    // Set the current working directory to the root directory.
    log_trace("Daemon: before chdir()");
    if (chdir(DAEMON_CHDIR) == -1) {
        log_error("Daemon: failed to change working directory while daemonising (errno=%d)",errno);
    }

    // Set the user file creation mask to zero.
    log_trace("Daemon: before umask()");
    umask(0);

    log_trace("Daemon: before close() FD 0,1,2");
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
    log_trace("Daemon: after reopen FD 0,1,2 set to /dev/null");

    log_debug("Daemon: Setting signal handlers");
    log_trace("Daemon: before signal handlers");

    setSignalHandlers();
    log_trace("Daemon: after signal handlers");
    log_trace("Daemon: before doWork()");

    return 0;
}
