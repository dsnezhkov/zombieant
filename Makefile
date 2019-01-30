CC=gcc
GO=go
CFLAGS= -Wall


SRC = $(CURDIR)/src
BIN = $(CURDIR)/bin
LIB = $(CURDIR)/lib

all: libctx nomain main main_extern main_ctor main_init main_weakref weakrefso testso goshim

goshim:
	$(GO) build -o $(LIB)/shim.so -buildmode=c-shared $(SRC)/shim.go

nomain:
	$(CC) $(CFLAGS) -nostartfiles $(SRC)/nomain.c -o $(BIN)/nomain

nomain_interp:
	$(CC) $(CFLAGS) -shared -Wl,-e,fn_no_main $(SRC)/nomain_interp.c -o $(BIN)/nomain_interp

testso:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/testso.c -Wl,-soname,libtestso.so -o $(LIB)/libtestso.so

weakrefso:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakrefso.c -o $(LIB)/weakrefso.so
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/weakrefso2.c -o $(LIB)/weakrefso2.so

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
	
psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c $(SRC)/psevade.c -o $(LIB)/psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o $(LIB)/libctx.so.1  $(LIB)/psevade.o   -ldl

client:
	$(CC) $(CFLAGS) $(SRC)/client_dyn.c -o $(BIN)/client_dyn -ldl

clean_psevade:
	$(RM) $(LIB)/psevade.o $(LIB)/libctx.so.1

clean_main:
	$(RM) $(BIN)/main $(BIN)/main_extern $(BIN)/main_ctor $(BIN)/main_init $(BIN)/nomain $(BIN)/nomain_interp $(BIN)/main_weakref

clean_testso:
	$(RM) $(LIB)/libtestso.so

clean_weakrefso:
	$(RM) $(LIB)/weakrefso.so $(LIB)/weakrefso2.so

clean_shims:
	$(RM) $(LIB)/shim.so

clean: clean_psevade clean_main clean_testso clean_shims clean_weakrefso 
