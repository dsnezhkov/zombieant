CC=gcc
CFLAGS= -Wall


SRC = $(CURDIR)/src
BIN = $(CURDIR)/bin
LIB = $(CURDIR)/lib

all: libctx nomain main main_extern main_ctor main_init  testso

nomain:
	$(CC) $(CFLAGS) -nostartfiles $(SRC)/nomain.c -o $(BIN)/nomain

testso:
	$(CC) $(CFLAGS) -fPIC -shared $(SRC)/testso.c -Wl,-soname,libtestso.so -o $(LIB)/libtestso.so

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
	$(RM) $(BIN)/main $(BIN)/main_extern $(BIN)/main_ctor $(BIN)/main_init $(BIN)/nomain

clean_testso:
	$(RM) $(LIB)/libtestso.so

clean: clean_psevade clean_main clean_testso
