CC=gcc
CFLAGS= -Wall


SRC = $(CURDIR)/src
BIN = $(CURDIR)/bin
LIB = $(CURDIR)/lib

all: libctx nomain main main_extern

nomain:
	$(CC) $(CFLAGS) -nostartfiles $(SRC)/nomain.c -o $(BIN)/nomain

main:
	$(CC) $(CFLAGS) $(SRC)/main.c -o $(BIN)/main

main_extern:
	$(CC) $(CFLAGS) $(SRC)/main_extern.c -o $(BIN)/main_extern -L $(LIB) -l:libctx.so.1
	
psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c $(SRC)/psevade.c -o $(LIB)/psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o $(LIB)/libctx.so.1  $(LIB)/psevade.o 

clean_psevade:
	$(RM) $(LIB)/psevade.o $(LIB)/libctx.so.1

clean_main:
	$(RM) $(BIN)/main $(BIN)/main_extern $(BIN)/nomain

clean: clean_psevade clean_main
