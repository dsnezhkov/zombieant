CC = gcc
GO = go
CFLAGS= -Wall -ggdb3 -O0


SRC = $(CURDIR)/src
BIN = $(CURDIR)/bin
LIB = $(CURDIR)/lib
EXT = $(CURDIR)/ext
LUAJITFLAGS = -I/root/Code/zombieant/ext/luadist/include/luajit-2.0 -L/root/Code/zombieant/ext/luadist/lib -lluajit-5.1


all: libctx nomain nomain_interp nomain_entry main main_extern main_ctor main_init main_weakref_control main_weakref_chained libweakref libmctor libinterrupt
all_plus_shims: all goshim

### Libs
libinit:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/init.c -Wl,-soname,libinit.so -o $(LIB)/libinit.so

libcstart: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/lcstart.c -Wl,-soname,libcstart.so -o $(LIB)/libcstart.so -ldl

libweakref_control: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref_control.c -Wl,-soname,libweakrefctrl.so -o $(LIB)/libweakrefctrl.so

libweakref_foreign: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref_foreign.c -Wl,-soname,libweakreffor.so -o $(LIB)/libweakreffor.so

libweakref_chained: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref_chained.c -Wl,-soname,libweakrefchain1.so -o $(LIB)/libweakrefchain1.so
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref_chained2.c -Wl,-soname,libweakrefchain2.so -o $(LIB)/libweakrefchain2.so
	
libmctor:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/mctor.c -Wl,-soname,libmctor.so -o $(LIB)/libmctor.so

libinterrupt:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/intrp.c -Wl,-soname,libinterrupt.so -o $(LIB)/libinterrupt.so

libweakref: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref.c -Wl,-soname,libweakref.so -o $(LIB)/libweakref.so
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref_chained2.c -Wl,-soname,libweakref2.so -o $(LIB)/libweakref2.so

libaslr: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/aslr.c -Wl,-soname,libaslr.so -o $(LIB)/libaslr.so

### Mains
main:
	$(CC) $(CFLAGS) -static-libgcc -static-libstdc++ $(SRC)/main.c -o $(BIN)/main -include /root/Code/zombieant/ext/glibc_version_header/version_headers/force_link_glibc_2.17.h

main_init:
	$(CC) $(CFLAGS) $(SRC)/main_init.c -o $(BIN)/main_init

main_weakref_control:
	$(CC) $(CFLAGS) -fPIC  $(SRC)/main_weakref_control.c -o $(BIN)/main_weakref_control

main_weakref_chained:
	$(CC) $(CFLAGS) -fPIC  $(SRC)/main_weakref_chained.c -o $(BIN)/main_weakref_chained

main_ctor:
	$(CC) $(CFLAGS) $(SRC)/main_ctor.c -o $(BIN)/main_ctor

main_ldprotect:
	$(CC) $(CFLAGS) -DLD_PROTECT $(SRC)/main_ldprotect.c -o $(BIN)/main_ldprotect

main_extern:
	$(CC) $(CFLAGS) $(SRC)/main_extern.c -o $(BIN)/main_extern -L $(LIB) -l:libctx.so.1
	
nomain:
	$(CC) $(CFLAGS) -nostartfiles $(SRC)/nomain.c -o $(BIN)/nomain

nomain_interp:
	$(CC) $(CFLAGS) -shared -Wl,-e,fn_no_main $(SRC)/nomain_interp.c -o $(BIN)/nomain_interp

nomain_entry:
	$(CC) $(CFLAGS) -nostartfiles -Wl,-e,__data_frame_e $(SRC)/nomain_entry.c -o $(BIN)/nomain_entry


#### ZombieAnt
psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c $(SRC)/psevade.c -o $(LIB)/psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o $(LIB)/libctx.so.1  $(LIB)/psevade.o   -ldl

client:
	$(CC) $(CFLAGS) $(SRC)/client_dyn.c -o $(BIN)/client_dyn -ldl

goshim:
	$(CC) $(CFLAGS) $(SRC)/dyload.c -o $(BIN)/dyload -ldl
	$(GO) build -o $(LIB)/shim.so -buildmode=c-shared $(SRC)/shim.go

#### Lua
luajit:
	$(CC) $(SRC)/loadlua.c -o $(BIN)/loadlua $(LUAJITFLAGS) -lm -ldl

luajitstatic: 
	$(CC) -static -static-libgcc -Wl,-E -o $(BIN)/loadlua src/loadlua.c -I$(EXT)/luadist-static/include/luajit-2.0 -L$(ext)/luadist-static/lib -L/lib/x86_64-linux-gnu $(EXT)/luadist-static/lib/libluajit-5.1.a -lm -ldl
	
luajitstatic_luastatic:
	$(CC) -Os $(SRC)/test.lua.c -o $(BIN)/test_lua_static $(EXT)/luadist-static/lib/libluajit-5.1.a -rdynamic -lm -ldl -I$(EXT)/luadist-static/include/luajit-2.0/ -static-libgcc -fPIC

#### ASLR weaken PoC

aslr_weaken:
	$(CC) -z execstack -o $(BIN)/aslr_cradle_template src/aslr_cradle_template.c
	$(CC) -o $(BIN)/aslr_weakener src/aslr_weakener.c


########### Clean
clean_psevade:
	$(RM) $(LIB)/psevade.o $(LIB)/libctx.so.1

clean_main:
	$(RM) $(BIN)/main $(BIN)/main_extern $(BIN)/main_ctor $(BIN)/main_init $(BIN)/nomain $(BIN)/nomain_interp $(BIN)/main_weakref $(BIN)/nomain_entry

clean_libmctor:
	$(RM) $(LIB)/libmctor.so

clean_libinterrupt:
	$(RM) $(LIB)/libinterrupt.so

clean_libweakref:
	$(RM) $(LIB)/libweakref.so $(LIB)/libweakref2.so

clean_shims:
	$(RM) $(LIB)/shim.so

clean_aslr_weaken:
	$(RM) $(BIN)/aslr_weakener $(BIN)/aslr_cradle_template

clean: clean_psevade clean_main clean_libmctor clean_libweakref clean_libinterrupt clean_shims
