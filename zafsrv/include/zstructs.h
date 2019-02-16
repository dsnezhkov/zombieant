#include "zparams.h"

#ifndef Z_STRUCT
#define Z_STRUCT

// Kernel Verison: memfd_create support
typedef struct {
    int major;
    int minor;
    int memfd_supp;
} memfd_kv;
memfd_kv memfd_kv_s;

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
node_t * head;

// Downloader Module entry
typedef struct {
 int  shm_fd;
 FILE *shm_file;
 char shm_fname[SHM_NAME_MAX];
 char shm_mname[6];
 int  shm_type; // 1 - shm, 2 - memfd
} Shared_Mem_Fd;

#endif
