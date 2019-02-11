
#ifndef _HAVE_ZAF_H
#define _HAVE_ZAF_H

typedef struct node {
    char mname[256];
    char mpath[1024];
    struct node * next;
} node_t;

node_t * head = NULL;
int push(node_t * head, char* path, char * mname);
int push_first(node_t * head, char* path, char * mname);
int check_empty(node_t * head);
void print_list(node_t * head);

typedef struct {
 int shm_fd;
} Shared_Mem_Fd;

#endif //_HAVE_ZAF_H

