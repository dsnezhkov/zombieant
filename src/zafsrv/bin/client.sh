#!/bin/bash

exec 3<>/dev/tcp/127.0.0.1/5001
echo "{\"command\": \"listmod\"}" 1>&3
timeout 1s /bin/cat <&3

