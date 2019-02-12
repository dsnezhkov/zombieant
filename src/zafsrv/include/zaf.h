#include "log.h"

#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H


#define MAX_BUF 1024  //command buffer
#define SLEEP_TIMER 5 //seconds

typedef struct modtbl {
    char mname[256];
    char mpath[1024];
} modtbl_t;

typedef struct node {
    char mname[256];
    char mpath[1024];
    struct node * next;
} node_t;

node_t * head = NULL;
int push(node_t * head, char* path, char * mname);
int push_first(node_t * head, char* path, char * mname);
int check_empty(node_t * head);
int list_sz(node_t * head);
void print_list(node_t * head);
void write_modlist(node_t * head, int sock);
int doprocessing (int sock, node_t * head);
int setupCmdP();

typedef struct {
 int shm_fd;
} Shared_Mem_Fd;

#endif //_HAVE_ZAF_H

