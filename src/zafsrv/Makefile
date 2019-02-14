LIBS=-ldl -lpthread
CFLAGS=-Os -Wall 
EXTROOT=/root/Code/zombieant/ext

SYMVER_CFLAGS=-Os -Wall -static-libgcc  -include $(EXTROOT)/glibc_version_header/version_headers/force_link_glibc_2.17.h

ZROOT=/root/Code/zombieant/src/zafsrv
ZBUILD=$(ZROOT)/build
ZBIN=$(ZROOT)/bin
ZLIB=$(ZROOT)/lib


# Ref: https://x-c3ll.github.io/posts/fileless-memfd_create/
zaf: log.o spt.o zaf.o
	gcc $(SYMVER_CFLAGS)  $(ZBUILD)/zaf.o $(ZBUILD)/log.o $(ZBUILD)/spt.o -o $(ZBIN)/zaf \
		$(ZLIB)/libcurl.a \
	   	$(ZLIB)/libssl.a \
		$(ZLIB)/libz.a \
		$(ZLIB)/libcjson.a \
		$(ZLIB)/libcrypto.a  $(LIBS) -lrt

log.o: log.c
	gcc $(SYMVER_CFLAGS) -c log.c -o$(ZBUILD)/log.o -I$(ZROOT)/include/

zaf.o: zaf.c 
	gcc $(SYMVER_CFLAGS) -c zaf.c -o$(ZBUILD)/zaf.o -I$(EXTROOT)/curl/include -I$(ZROOT)/include -include $(EXTROOT)/glibc_version_header/version_headers/force_link_glibc_2.17.h

spt.o: spt_status.c
	gcc $(SYMVER_CFLAGS) -c spt_status.c -o $(ZBUILD)/spt.o -I$(ZROOT)/include

clean:
	$(RM) $(ZBUILD)/*

.PHONY: clean
