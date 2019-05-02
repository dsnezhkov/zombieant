#!/usr/bin/env python3

import sys
import os
import getopt
from ctypes import *

def weaken():
   libc = CDLL("libc.so.6")
   ADDR_NO_RANDOMIZE = 0x004000
   saved_persona = libc.personality (0xffffffff)
   libc.personality (saved_persona | ADDR_NO_RANDOMIZE);

def lamb(extso, target = []):
   print('\nIn child ',  os.getpid())
   weaken()

   if len(extso) > 0:
      os.environ["LD_PRELOAD"] = extso

   os.execve(target[0], target, os.environ)

def sacrifice(extso, target = []):
   newpid = os.fork()
   if newpid == 0:
      lamb (extso, target) 
   else:
      pids = (os.getpid(), newpid)
      print("parent: %d, child: %d\n" % pids)



def usage(err_str=""):
    print(

    """
    error: %s

    Usage: %s [-l <lib.so> ] -t <target>
                -h: help
                -l: path to /external/lib.so
                -t: path to /target/to/weaken 

    Example:./setpersona.py  -l ./libaslr.so  -t /bin/ls

    """ % (err_str,sys.argv[0])
    )
    sys.exit()



def main():

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hl:t:", 
                ["help", "target=", "lpath="])
    except getopt.GetoptError as err:
        usage(str(err))
    
    target     = None
    extso      = None

    for o, a in opts:
        if   o in ("-h", "--help"):
             usage()
        elif o in ("-l", "--lpath"):
             extso = a
        elif o in ("-t", "--target"):
             target = a
        else:
            usage("Unhandled option");
    
    if len(sys.argv) < 2:
        usage("Invalid number of args")

    print("Args left:", args)
    print("Command ", target)

    if target == None:
       usage("Target?")

    target_w_args = [target] + args
    print("Target : ", target_w_args)

    if extso == None:
        extso = ""
        
    sacrifice(extso, target_w_args)

if __name__ == '__main__':
    main()
