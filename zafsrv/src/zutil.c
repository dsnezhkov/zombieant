#include "zutil.h"

// Detect if kernel is < or => than 3.17
// Op_nomad: modded fallback, syscall checks
//
// return 0 - POSIX shm only , 1 - memfd_create supported
// 
int setKernel() {

    struct utsname buffer;
    char   *token;
    char   *separator = ".";
    int    memfdcheck;
       

    if (uname(&buffer) != 0) {
      log_error("KVersion: Unable to use utsname. Assume fallback");
    }

    log_info("\tsystem name = %s", buffer.sysname);
    log_info("\tnode name   = %s", buffer.nodename);
    log_info("\trelease     = %s", buffer.release);
    log_info("\tversion     = %s", buffer.version);
    log_info("\tmachine     = %s", buffer.machine);

    // Quick 
    memfdcheck = syscall(SYS_memfd_create, "", 1);
    if (memfdcheck != -1 ){
        close(memfdcheck);
        memfdcheck = 1; // memfd_create syscall available
    }

    token = strtok(buffer.release, separator);
    memfd_kv_s.major = atoi(token);
    log_debug("KVersion: release major: %d", memfd_kv_s.major);

    token = strtok(NULL, separator);
    memfd_kv_s.minor = atoi(token);
    log_debug("KVersion: release minor: %d", memfd_kv_s.minor);

    if (memfd_kv_s.major < 3) {
       memfd_kv_s.memfd_supp = 0;   // shm only
       return 0;
    }
    else if ( memfd_kv_s.major > 3){
      if (memfdcheck == 1) {
          memfd_kv_s.memfd_supp = 1; // TODO: WARNING!!!! only set to 0 for shm testing. change it back to 1 for proper.
          return 1;                  // TODO: WARNING!!!! only set to 0 for shm testing. change it back to 1 for proper.
      }
      memfd_kv_s.memfd_supp = 0;
    }

    if ( memfd_kv_s.minor < 17) {
        memfd_kv_s.memfd_supp = 0;   // shm only
        return 0;
    }
    else {
      if (memfdcheck == 1) {    
          memfd_kv_s.memfd_supp = 1;  // TODO: WARNING!!!! only set to 0 for shm testing. change it back to 1 for proper.
          return 1;                   // TODO: WARNING!!!! only set to 0 for shm testing. change it back to 1 for proper.
      }
      memfd_kv_s.memfd_supp = 0;
      return 0;
    }
}

// TODO: implement true rand
// TODO: paramterize size of string
void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

void cleanup(int cleanLog){
    struct stat sb;

    if (fp != NULL){
        fclose (fp);
    }

    if (cleanLog) {
        if (lstat(logFile, &sb) != -1){
            unlink(logFile);
        }
    }
}
