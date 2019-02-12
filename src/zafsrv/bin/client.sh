#!/bin/bash

PORT=$(lsof -i | grep zaf | awk -F: '{print $2}' | sed 's/ (LISTEN)//')
if [[ ! -z $PORT ]]
then
    exec 3<>/dev/tcp/127.0.0.1/$PORT
    echo -n "{\"command\": \"listmod\"}" 1>&3
    timeout 1s /bin/cat <&3
else
    echo "Check ZAF"
fi
