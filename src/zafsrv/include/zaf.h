#include "log.h"
#include "spt_status.h"


#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H


#define MAX_BUF 1024  //command buffer
#define SLEEP_TIMER 5 //seconds



// ShMemTbl List
typedef struct node {
    char mname[256];
    char mpath[1024];
    struct node * next;
} node_t;
typedef struct modtbl {
    char mname[256];
    char mpath[1024];
} modtbl_t;
node_t * head = NULL;


// Curl Shared Memory write callback
typedef struct {
 int shm_fd;
} Shared_Mem_Fd;

// Signals
/*  
 * 1 - SIGHUP
 * 2 - SIGUSR1
 * 3 - ...
 */
volatile sig_atomic_t sigflag  = 0; 
void setSignalHandlers();
void handleSig(int signo);
void doSigHup(void);
void doSigUsr1(void);

// Functions
int  push(node_t * head, char* path, char * mname);
int  push_first(node_t * head, char* path, char * mname);
int  check_empty(node_t * head);
int  list_sz(node_t * head);
void write_modlist(node_t * head, int sock);
int  processCommandReq (int sock, node_t * head);
int  backgroundDaemonLeader();
int  setupCmdP();

#endif //_HAVE_ZAF_H

