#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#include <curl/curl.h>

#include "log.h"
#include "zparams.h"
#include "zstructs.h"
#include "zmemfd.h"


extern int url2fd(char *downloadUrl, Shared_Mem_Fd * smf);
extern int setMemfdTbl(int shm_fd, char* mname);
extern int open_ramfs(Shared_Mem_Fd * smf);



int  check_empty(node_t * head);
int  list_sz(node_t * head);
int  mod_name2fd(node_t * head, char * name, int loaded);
void write_modlist(node_t * head, int sock);
int  unload_mod(node_t * head, char * name);
int  find_mod(node_t * head, char * name);
int  load_mod(node_t * head, char * url);
int  mark_delete_mod(node_t * head, int fd);
int  push_first(node_t * head, int shm_fd, char* path, char* mname, char mstate[]);
int  push(node_t * head, int shm_fd, char* path, char* mname, char mstate[]);
void cleanup_mod_resources(int shm_fd);

