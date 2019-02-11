LIBS=-ldl -lpthread
CFLAGS=-Os 
ROOT=/root/Code/zombieant/src/zafsrv
LIBROOT=$(ROOT)/lib
EXTROOT=/root/Code/zombieant/ext



cget: cget.c
	gcc $(CFLAGS) $< -o $@ -I$(EXTROOT)/curl/include \
	   	$(EXTROOT)/curl/lib/libcurl.a \
	   	$(EXTROOT)/openssl/lib/libssl.a \
	   	$(EXTROOT)/zlib/lib/libz.a \
	   	$(EXTROOT)/openssl/lib/libcrypto.a $(LIBS)

# Ref: https://x-c3ll.github.io/posts/fileless-memfd_create/
getinmemory2: getinmemory2.c
	gcc $(CFLAGS) $< -o $@ -I$(EXTROOT)/curl/include \
		$(LIBROOT)/libcurl.a \
	   	$(LIBROOT)/libssl.a \
		$(LIBROOT)/libz.a \
		$(LIBROOT)/libcrypto.a $(LIBS) -lrt

clean:
	$(RM) cget getmemory getinmemory2
