CC=gcc
CFLAGS= -Wall


all: libctx

psevade.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c psevade.c -o psevade.o

libctx: psevade.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o libctx.so.1.0  psevade.o 

clean:
	$(RM) psevade.o libctx.so.1.0
