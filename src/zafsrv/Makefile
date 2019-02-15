
CFLAGS=-Os -Wall -static 

EXTROOT=/root/Code/zombieant/ext
ZROOT=/root/Code/zombieant/src/zafsrv
ZBUILD=$(ZROOT)/build
ZEXSRC=$(ZROOT)/extsrc
ZBIN=$(ZROOT)/bin
ZLIB=$(ZROOT)/lib

LIBS=-lrt -lpthread 


zaf: log.o spt.o zaf.o
	gcc $(CFLAGS) \
		$(ZBUILD)/zaf.o $(ZBUILD)/log.o $(ZBUILD)/spt.o $(ZBUILD)/zsignals.o \
		$(ZBUILD)/zservice.o $(ZBUILD)/zutil.o \
		-o $(ZBIN)/zaf \
		$(ZLIB)/libcurl.a \
	   	$(ZLIB)/libssl.a \
		$(ZLIB)/libcjson.a \
		$(ZLIB)/libcrypto.a  $(LIBS)

zaf.o: zsignals.o zservice.o 
	gcc $(CFLAGS) -c zaf.c -o$(ZBUILD)/zaf.o -I$(ZROOT)/../../ext/curl/include -I$(ZROOT)/include 

zsignals.o: 
	gcc $(CFLAGS) -c zsignals.c -o$(ZBUILD)/zsignals.o -I$(ZROOT)/include/

zservice.o:
	gcc $(CFLAGS) -c zservice.c -o$(ZBUILD)/zservice.o -I$(ZROOT)/include/

zutil.o:
	gcc $(CFLAGS) -c zutil.c -o$(ZBUILD)/zutil.o -I$(ZROOT)/include/



log.o: 
	gcc $(CFLAGS) -c $(ZEXSRC)/log.c -o$(ZBUILD)/log.o -I$(ZROOT)/include/

spt.o:
	gcc $(CFLAGS) -c $(ZEXSRC)/spt_status.c -o $(ZBUILD)/spt.o -I$(ZROOT)/include

clean:
	$(RM) $(ZBUILD)/*

.PHONY: clean
