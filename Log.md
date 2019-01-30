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


## TODO
- fully clean LD_PRELOAD  in ctor of an injector )libctx)
- weak attributes modules load from libctx
- multilib loading in LD_PRELOAD
- __gmon_start__ (w) https://github.com/lattera/glibc/blob/master/csu/gmon-start.c
	[ref] (https://stackoverflow.com/questions/12697081/what-is-gmon-start-symbol)
