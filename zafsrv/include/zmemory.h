#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "zstructs.h"
#include "log.h"


#define MFD_ALLOW_SEALING 1

extern void gen_random(char *s, const int len);
extern int  memfd_create(const char *name, unsigned int flags);

int open_ramfs(Shared_Mem_Fd * smf);
