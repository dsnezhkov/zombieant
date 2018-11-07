#!/bin/bash


usage(){
	echo 
	echo "run <-b|-B|-f> <metacommand> <trampoline exe>"
	echo " -b backgrounding with std(in|out|err) left open"
	echo " -B backgrounding with std(in|out|err) closed"
	echo " -f foreground"
	echo 
	echo " metacommand:"
	echo "	'r:name (str)' # immediate rename to a static name "
	echo "	'R:name (str)' # delayed rename to a static name "
	echo "	'M:key  (int)' # delayed rename to a dynamic name "
	echo 
	echo "Ex.: ./run.sh  -b r:[rpcd] ./main"
	exit 1
}
make clean
make

if [ $# -ne 3 ]
  then
    usage
fi


while getopts ":b:B:f" opt; do
  case $opt in
    b)
		LD_BG=true  LD_CMD=$2 LD_PRELOAD="${PWD}/lib/libctx.so.1" $3
      ;;
    B)
		LD_BG="fdclose"  LD_CMD=$2 LD_PRELOAD="${PWD}/lib/libctx.so.1" $3
      ;;
    f)
		LD_CMD=$2 LD_PRELOAD="${PWD}/lib/libctx.so.1" $3
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

