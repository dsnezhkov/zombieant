#include <string.h>
#include "log.h"
#include <curl/curl.h>
#include "zstructs.h"


#define ZCURL_AGENT "libcurl-agent/1.0"    // UA-Agent

extern int open_ramfs(Shared_Mem_Fd * smf);
extern void cleanup_mod_resources(int shm_fd);
