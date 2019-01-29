# psevade

`gcc -DPSDEBUG -fpic -shared -o stuff.so stuff.c`

	or:
`gcc -Wall -fPIC -DPSDEBUG -c stuff.c`
`gcc -shared -Wl,-soname,libctx.so.1 -o libctx.so.1.0 stuff.o`

`gcc -o main main.c`

`LD_PRELOAD=./stuff.so ./main arg`
`LD_PRELOAD=./stuff.so /bin/cat`
`DYLD_INSERT_LIBRARIES=./stuff.so ./main arg`

`./psrename 196609 "ping 127.0.0.1"`

`kill -SIGUSR1  <main_pid>`

`ipcs -a |grep 666 | grep -v 0x41010002 | awk '{print $2}' | xargs -i ipcrm -q {}`
`ipcrm -q  163843`

`make clean && make all && LD_PRELOAD=./libctx.so.1.0 wc -l /dev/urandom  @M:shm@`

`https://www.cprogramming.com/tutorial/shared-libraries-linux-gcc.html`
`https://stackoverflow.com/questions/32700494/executing-init-and-fini`

refer:
`https://haxelion.eu/article/LD_NOT_PRELOADED_FOR_REAL/`


` /lib64/ld-linux-x86-64.so.2  ./elfloader  hello.so  main1 `

`LD_PCMD="r:smtp" LD_MODULE="./lib/shim.so" LD_MODULE_ARGS="hello" LD_PRELOAD=./lib/libctx.so.1 /bin/ls`
