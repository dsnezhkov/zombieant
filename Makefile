CC=gcc
CFLAGS= -Wall


all: libctx nomain main main_extern

nomain:
	$(CC) $(CFLAGS) -nostartfiles nomain.c -o nomain

main:
	$(CC) $(CFLAGS) main.c -o main

main_extern:
	$(CC) $(CFLAGS) main_extern.c -o main_extern -L $(CURDIR) -l:libctx.so.1
	
psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c psevade.c -o psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o libctx.so.1  psevade.o 

clean_psevade:
	$(RM) psevade.o libctx.so.1

clean_main:
	$(RM) main main_extern nomain

clean: clean_psevade clean_main
