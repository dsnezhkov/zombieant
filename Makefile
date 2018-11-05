CC=gcc
CFLAGS= -Wall


all: libctx

stuff.o:
	$(CC) $(CFLAGS) -fPIC -DPSDEBUG -c stuff.c -o stuff.o

libctx: stuff.o 
	$(CC) $(CFLAGS) -shared -Wl,-soname,libctx.so.1 -o libctx.so.1.0  stuff.o 

clean:
	$(RM) stuff.o libctx.so.1.0
