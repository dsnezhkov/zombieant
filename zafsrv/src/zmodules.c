#include "zmodules.h"

// TODO: try to implement node deletion 
int check_empty(node_t * head){
    int empty  = (head == NULL) ? 0 : 1;
    return empty;
}

int list_sz(node_t * head) {
    node_t * current = head;
    int c = 1;
    while (current != NULL) {
        c++;
        current = current->next;
    }
    return c;
}

int mod_name2fd(node_t * head, char * name, int loaded) {
    node_t * current = head;
    int found=0;

    while (current != NULL) {
        if ( strncmp(current->mname, name, sizeof(current->mname)) == 0){
          log_debug("FindMod: found entry %s", current->mname);

          if (loaded){
              if (strncmp(current->mstate,"U", strlen("U")) == 0 ){
                  current = current->next;
                  continue; 
              }
              found=1;
              break;
              
          }else{
            found=1;
            break;
          }
        }
        current = current->next;
    }

    if (found){
        return current->fd; 
    }
    return 0;
}

char * mod_name2path(node_t * head, char * name) {
    node_t * current = head;
    char * empty = "";
    int found=0;

    while (current != NULL) {
        if ( strncmp(current->mname, name, sizeof(current->mname)) == 0){
          log_debug("FindMod: found entry %s", current->mname);
          found=1;
          break;
        }
        current = current->next;
    }

    if (found){
        return strdup(current->mpath); 
    }
    return empty;
}

void write_modlist(node_t * head, int sock) {
    node_t * current = head;
    int n, ret;
    char * entry;

    if (current == NULL) {
        n = write(sock,"",1);
        if (n < 0)
          log_error("Error writing to socket");
    }

    while (current != NULL) {
        entry = calloc(1, MAX_BUF);

		ret = snprintf(entry, MAX_BUF-1, "%d :(%s): %s : %s\n", current->fd, current->mstate, current->mpath, current->mname);
        if (ret < 0) {
         free(entry);
         continue;
        }

        n = write(sock,entry,strlen(entry));

        if (n < 0) {
          log_error("Error writing to socket");
        }
        free(entry);
        current = current->next;
    }
}

int unload_mod(node_t * head, char * name) {
    int modfd   = 0;
    int status  = 0;
    int loaded  = 1; // TODO: why is it here? 

    modfd = mod_name2fd(head, name, loaded);
    if ( modfd != 0 ){
       log_debug("Worker: Module fd found %d", modfd);
       status = 1;
       log_debug("Worker: Closing OS process fd %d", modfd);
       cleanup_mod_resources(modfd);
       //log_debug("Worker: Removing data from memory for fd %d", modfd);
       //mark_delete_mod(head, modfd); // from ShmMemTbl
    }else{
       log_debug("Worker: Module fd not found");
    }

    return status;
}

int find_mod(node_t * head, char * name) {
    
    char* modname   = NULL;
    int   status    = 0;

    modname = mod_name2path(head, name);
    if ((modname != NULL) && (modname[0] != '\0')) {
        log_debug("Worker: Module found %s", modname);
        status = 1;
    }else{
        log_debug("Worker: Module not found %s", modname);
    }

    return status;
}

// TODO: is load the same as activate? Should we have it as separate action?
int load_mod(node_t * head, char * url) {

    CURLU       *h;
    CURLUcode   uc;
    char        *path=NULL, *fileName=NULL;
    int         uresp=0, sresp=0;

    Shared_Mem_Fd shm_fd_s = {
        .shm_fd=0, 
        .shm_file=NULL,
        .shm_fname={0}, 
        .shm_mname={0}, 
        .shm_type=0 
    };

    h = curl_url();
    if(!h){
        log_debug("LoadMod: Unable to get cURL object\n");
        return 1;
    }

    uc = curl_url_set(h, CURLUPART_URL, url, 0);
    if(uc)
      goto clean;


    // Get file name
    uc = curl_url_get(h, CURLUPART_PATH, &path, 0);
    if(!uc) {
        log_debug("Path: %s (%d)b\n", path, strlen(path));
        if (path[0] == '/') {// remove slash
            fileName = calloc(strlen(path)-1, sizeof(char));
            memcpy(fileName, path+1, strlen(path)-1);
        }
    }
    //
    // Save fileName in object record
    if ( strlen(fileName) < SHM_NAME_MAX ){
        memcpy(shm_fd_s.shm_fname, fileName, strlen(fileName) );
    }else{
        memcpy(shm_fd_s.shm_fname, fileName, SHM_NAME_MAX -1 );
    }

    // TODO: decide between shm_fd and status as return since we set up shm_fd via the struct passed in
	sresp = open_ramfs(&shm_fd_s); // Give me a file descriptor to memory

    if ( sresp == 0 ){ // memory not allocated - return
	    log_debug("RamWorker: File Descriptor Shared Memory NOT created! Bailing.");
        return sresp;
    }

    // Download file to memory
    log_debug("LoadMod: Downloading %s -> %s", url, path);
    uresp = url2fd(url, &shm_fd_s);
    if (uresp != 0){
        log_debug("LoadMod: fetched OK, setting Mem table");
        setMemfdTbl(shm_fd_s.shm_fd, fileName);
    }else{
        log_debug("LoadMod: url2fd failed, reclaiming resources");
        cleanup_mod_resources(shm_fd_s.shm_fd);
    }

clean:
    free(fileName);
    curl_free(path);
    curl_url_cleanup(h); /* free url handle */

    return 0;
}

//delete a link with given key
// TODO: implement size and type in status
int mark_delete_mod(node_t * head, int fd) {

   node_t * current = head;
   int status       = 0;
   char state[]     = "U";

   if(head == NULL)
      return status;

   while (current != NULL) {
      if ( current->fd == fd ){ // found
          strncpy(current->mstate, state, strlen(state));
      }
      current = current->next;
   }

   return status;
}

int push_first(node_t * head, int shm_fd, char* path, char* mname, char mstate[]) {

    log_debug("ModTable: Adding path %s, strlen(path): %d name: %s strlen(name): %d", path, strlen(path), mname, strlen(mname));
    head->fd = shm_fd;
    strncpy(head->mpath, path, strlen(path));
    strncpy(head->mname, mname, strlen(mname));
    strncpy(head->mstate, mstate, strlen(mstate));
    head->next = NULL;
    log_debug("ModTable: Added: %s : %s", head->mname, head->mpath);

    return 0;
}

int push(node_t * head, int shm_fd, char* path, char* mname, char mstate[]) {
    node_t * current = head;

    log_debug("ModTable: Adding path %s, strlen(path): %d name: %s strlen(name): %d", path, strlen(path), mname, strlen(mname));

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = calloc(1,sizeof(node_t));
    if (current == NULL) {
        log_fatal("ModTable: calloc() failed");
        return 1;
    }

    current->next->fd = shm_fd;
    strncpy(current->next->mpath, path, strlen(path));
    strncpy(current->next->mname, mname, strlen(mname));
    strncpy(current->next->mstate, mstate, strlen(mstate));
    current->next->next = NULL;
    log_debug("ModTable: entry: %s : %s", current->mname, current->mpath);

    return 0;
}

void cleanup_mod_resources(int shm_fd){

    char procpath[1024]; // TODO: what should this be? parameterize
    char *shmpath;
    struct stat sb;
    

    // shm option: cleanup lingering file in /dev/shm
    if (memfd_kv_s.memfd_supp == 0) { 
        snprintf(procpath, 1024, "/proc/%d/fd/%d", getpid(), shm_fd);

        if (lstat(procpath, &sb) != -1){
            shmpath = realpath(procpath, NULL);
            if (shmpath != NULL)
            {
                log_info("Cleanup: unlinking %s -> %s", procpath, shmpath);
                unlink(shmpath); // TODO: make provisions for shm_unlink
                free(shmpath);
            }
        }
    }
    
    // memfd and shm options;
    log_debug("Cleanup: Closing shm_fd: %d", shm_fd);
    close(shm_fd); 

    // TODO: needed? mark unloaded in ShmMemTbl
    mark_delete_mod(head, shm_fd);
}

int setMemfdTbl(int shm_fd, char* mname) {
    char path[1024]; // TODO: what should this be?
    char mstate[] = "L"; //Loaded
    

    log_debug("MemTbl: Populating Index Table");
    if (memfd_kv_s.memfd_supp == 1) { 

	    log_debug("MemTbl: memfd_create() is supported.");
	    log_debug("MemTbl: Sealing modules.");
        /*
        // TODO: Allow commands to seal the modules that would never be modified
        // fcntl() defines as kernel 3.17 support. check for that.
        //unsigned int seals =  F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_WRITE | F_SEAL_SEAL;
        unsigned int seals =  F_SEAL_SEAL;
        if (fcntl(shm_fd, F_ADD_SEALS, seals) == -1)
	        log_error("MemTbl: fcntl(): Sealing modules failed"); // TODO: why failing?
        */

        snprintf(path, 1024, "/proc/%d/fd/%d", getpid(), shm_fd);
    } else { 
        snprintf(path, 1024, "/proc/%d/fd/%d", getpid(), shm_fd); 
	    log_debug("MemTbl: only /dev/shm supported.");
    }

    if (check_empty(head) == 0) {
        log_debug("Memtbl: Head of list empty");
        head = calloc(1,sizeof(node_t));
        if (head == NULL) {
		    log_fatal("MemTble: calloc() failed");
            return 1;
        }
        push_first(head, shm_fd, path, mname, mstate);
    }else{
        push(head, shm_fd, path, mname, mstate);
    }
    return 0;
}
