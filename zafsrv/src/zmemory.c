#include "zmemory.h"

// Returns a file descriptor where we can write our shared object
// TODO: refactor
int open_ramfs(Shared_Mem_Fd * smf) {

    int shm_fd = 0;
    char s[6] = {}; //TODO: parameterize

    gen_random(s, 6);//TODO: parameterize 

	if (memfd_kv_s.memfd_supp == 0) {
		shm_fd = shm_open(s, O_RDWR | O_CREAT, S_IRUSR|S_IRUSR|S_IRGRP|S_IRGRP|S_IRGRP|S_IRGRP);
		if (shm_fd < 0) { 
			log_fatal("RamWorker: shm_fd: Could not open file descriptor");
			return 0;
		}
        smf->shm_type=1; //shm
	}
	else {
		shm_fd = memfd_create(s, MFD_ALLOW_SEALING); // flags?
		if (shm_fd < 0) { 
			log_fatal("RamWorker: memfd_create: Could not open file descriptor\n");
			return 0;
		}
        smf->shm_type=2; //memfd
	}

    smf->shm_fd=shm_fd;
    memcpy(smf->shm_mname, s, 6);//TODO: parameterize
	return shm_fd;
}

