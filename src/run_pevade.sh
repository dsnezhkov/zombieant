#!/bin/bash

# python src/pypreload.py /bin/bash `pwd`/bin/main_init 

usage(){
	echo 
	echo "run_pevade <-r> <decoy name> <payload exe>"
	echo " -r randomize name from a pool of decoys"
	echo 
	echo " Note: payload path needs to be absolute or relative"
	echo "       but not include shell constructs or navigation"
	echo "       Ex. ./ ../ ~ "
	echo 
	echo 
	echo "Ex.: ./run.pevade /bin/bash bin/main"
	exit 1
}

if [ $# -lt 2 ]
  then
    usage
fi


while getopts ":r" opt; do
  case $opt in
    r)
        # randomize decoy
		
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

PYTHON="/usr/bin/python"
PYPRELOAD="./src/pypreload.py"
PDECOY=$1
PNAME=$2

shift;shift 

$PYTHON $PYPRELOAD $PDECOY $PNAME $*





