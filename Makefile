CC=gcc
GO=go
CFLAGS= -Wall


SRC = $(CURDIR)/src
BIN = $(CURDIR)/bin
LIB = $(CURDIR)/lib

all: libctx nomain main main_extern main_ctor main_init main_weakref libweakref libmctor libinterrupt
all_plus_shims: all goshim

### Libs
libmctor:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/mctor.c -Wl,-soname,libmctor.so -o $(LIB)/libmctor.so

libinterrupt:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/intrp.c -Wl,-soname,libinterrupt.so -o $(LIB)/libinterrupt.so

libweakref: 
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref.c -Wl,-soname,libweakref.so -o $(LIB)/libweakref.so
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakref2.c -Wl,-soname,libweakref2.so -o $(LIB)/libweakref2.so

### Mains
main_weakref:
	$(CC) $(CFLAGS) -fPIC  $(SRC)/main_weakref.c -o $(BIN)/main_weakref

main:
	$(CC) $(CFLAGS) $(SRC)/main.c -o $(BIN)/main

main_ctor:
	$(CC) $(CFLAGS) $(SRC)/main_ctor.c -o $(BIN)/main_ctor

main_init:
	$(CC) $(CFLAGS) $(SRC)/main_init.c -o $(BIN)/main_init

main_init_protect:
	$(CC) $(CFLAGS) -DLD_PROTECT $(SRC)/main_init.c -o $(BIN)/main_init

main_extern:
	$(CC) $(CFLAGS) $(SRC)/main_extern.c -o $(BIN)/main_extern -L $(LIB) -l:libctx.so.1
	
nomain:
	$(CC) $(CFLAGS) -nostartfiles $(SRC)/nomain.c -o $(BIN)/nomain

nomain_interpreter:
	$(CC) $(CFLAGS) -shared -Wl,-e,fn_no_main $(SRC)/nomain_interp.c -o $(BIN)/nomain_interpreter


#### ZombieAnt
psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c $(SRC)/psevade.c -o $(LIB)/psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o $(LIB)/libctx.so.1  $(LIB)/psevade.o   -ldl

client:
	$(CC) $(CFLAGS) $(SRC)/client_dyn.c -o $(BIN)/client_dyn -ldl

goshim:
	$(GO) build -o $(LIB)/shim.so -buildmode=c-shared $(SRC)/shim.go

########### Clean
clean_psevade:
	$(RM) $(LIB)/psevade.o $(LIB)/libctx.so.1

clean_main:
	$(RM) $(BIN)/main $(BIN)/main_extern $(BIN)/main_ctor $(BIN)/main_init $(BIN)/nomain $(BIN)/nomain_interp $(BIN)/main_weakref

clean_libmctor:
	$(RM) $(LIB)/libmctor.so

clean_libinterrupt:
	$(RM) $(LIB)/libinterrupt.so

clean_libweakref:
	$(RM) $(LIB)/libweakref.so $(LIB)/libweakref2.so

clean_shims:
	$(RM) $(LIB)/shim.so

clean: clean_psevade clean_main clean_libmctor clean_libweakref clean_libinterrupt clean_shims
