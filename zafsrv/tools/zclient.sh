#!/bin/bash

listmod(){

    echo '{"command": "listmod"}' | nc localhost $PORT
}

listmod_cat(){

    exec 3<>/dev/tcp/127.0.0.1/$PORT

cat<<! 1>&3
{"command": "listmod"}
!

    timeout 0.5s /bin/cat <&3
}

loadmod(){
    echo  "{ \"command\": \"loadmod\", \"args\": [ { \"modurl\": \"$1\" } ] }"  | nc localhost $PORT
}
loadmod_cat(){
    exec 3<>/dev/tcp/127.0.0.1/$PORT

cat<<! 1>&3
{ 
 "command": "loadmod", 
 "args": [ 
       { "modurl": "$1" } 
  ] 
}"
!

    timeout 1s /bin/cat <&3
}

unloadmod(){
    echo "{ \"command\": \"unloadmod\", \"args\": [ { \"modname\": \"$1\" } ] }" | nc localhost $PORT
}

unloadmod_cat(){
    exec 3<>/dev/tcp/127.0.0.1/$PORT

cat<<! 1>&3
{ 
 "command": "unloadmod", 
 "args": [ 
       { "modname": "$1" } 
  ] 
}"
!

    timeout 1s /bin/cat <&3
}


CMD=$1
CMDARG=$2
if [[ $# -lt 1 ]]
then
    echo "Usage: $0 <cmd> [cmd args]"
    echo "Example: $0 listmod"
    echo "Example: $0 loadmod http://127.0.0.1:8080/hax.so"
    echo "Example: $0 unloadmod hax.so"
    exit 1
fi

PORT=$(lsof -p  $(ps -ef | grep zaf$  | egrep -v '(tmux|egrep)' | awk '{print $2}' ) | grep '(LISTEN)' | awk -F: '{print $2}' | sed 's/ (LISTEN)//')
if [[ ! -z $PORT ]]
then
    #listmod
    #loadmod "http://127.0.0.1:8080/hax.so"
    $CMD $CMDARG
else
    echo "Check ZAF"
fi
