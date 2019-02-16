#include <string.h>
#include <unistd.h>
#include <cJSON.h>


#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#include "zstructs.h"
#include "log.h"


extern void write_modlist(node_t * head, int sock);
extern int  unload_mod(node_t * head, char * name);
extern int  load_mod(node_t * head, char * url);


int processCommandReq (int sock, node_t * head);
