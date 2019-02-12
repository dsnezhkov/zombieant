LIBS=-ldl -lpthread
CFLAGS=-Os

EXTROOT=/root/Code/zombieant/ext
ZROOT=/root/Code/zombieant/src/zafsrv
ZBUILD=$(ZROOT)/build
ZBIN=$(ZROOT)/bin
ZLIB=$(ZROOT)/lib


# Ref: https://x-c3ll.github.io/posts/fileless-memfd_create/
zaf: log.o zaf.o
	gcc $(CFLAGS) $(ZBUILD)/zaf.o $(ZBUILD)/log.o -o $(ZBIN)/zaf \
		$(ZLIB)/libcurl.a \
	   	$(ZLIB)/libssl.a \
		$(ZLIB)/libz.a \
		$(ZLIB)/libcjson.a \
		$(ZLIB)/libcrypto.a  $(LIBS) -lrt

log.o: log.c
	gcc -c log.c -o$(ZBUILD)/log.o -I$(ZROOT)/include/

zaf.o: zaf.c 
	gcc $(CFLAGS) -c zaf.c -o$(ZBUILD)/zaf.o -I$(EXTROOT)/curl/include -I$(ZROOT)/include

clean:
	$(RM) $(ZBUILD)/*

.PHONY: clean
