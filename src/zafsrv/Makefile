
CFLAGS=-Os -Wall -static 

EXTROOT=/root/Code/zombieant/ext
ZROOT=/root/Code/zombieant/src/zafsrv
ZBUILD=$(ZROOT)/build
ZBIN=$(ZROOT)/bin
ZLIB=$(ZROOT)/lib

LIBS=-lrt -lpthread 


zaf: log.o spt.o zaf.o
	gcc $(CFLAGS) $(ZBUILD)/zaf.o $(ZBUILD)/log.o $(ZBUILD)/spt.o $(ZBUILD)/zsignals.o -o $(ZBIN)/zaf \
		$(ZLIB)/libcurl.a \
	   	$(ZLIB)/libssl.a \
		$(ZLIB)/libcjson.a \
		$(ZLIB)/libcrypto.a  $(LIBS)

log.o: log.c
	gcc $(CFLAGS)  -c log.c -o$(ZBUILD)/log.o -I$(ZROOT)/include/

zsignals.o: zsignals.c
	gcc $(CFLAGS)  -c zsignals.c -o$(ZBUILD)/zsignals.o -I$(ZROOT)/include/

zaf.o: zsignals.o zaf.c 
	gcc $(CFLAGS)  -c zaf.c -o$(ZBUILD)/zaf.o -I$(ZROOT)/../../ext/curl/include -DCURL_STATIC -I$(ZROOT)/include 

spt.o: spt_status.c
	gcc  $(CFLAGS) -c spt_status.c -o $(ZBUILD)/spt.o -I$(ZROOT)/include

clean:
	$(RM) $(ZBUILD)/*

.PHONY: clean
