
CFLAGS=-Os -Wall -static 

EXTROOT=/root/Code/zombieant/ext
ZROOT=/root/Code/zombieant/zafsrv
ZBUILD=$(ZROOT)/build
ZSRC=$(ZROOT)/src
ZEXSRC=$(ZROOT)/extsrc
ZBIN=$(ZROOT)/bin
ZLIB=$(ZROOT)/lib

LIBS=-lrt -lpthread 

.SILENT:

zaf: log.o spt.o zaf.o
	gcc $(CFLAGS) \
		$(ZBUILD)/zaf.o $(ZBUILD)/log.o $(ZBUILD)/spt.o $(ZBUILD)/zsignals.o \
		$(ZBUILD)/zservice.o $(ZBUILD)/zutil.o $(ZBUILD)/zmodules.o $(ZBUILD)/zcommander.o \
		$(ZBUILD)/zmemory.o $(ZBUILD)/zdownloader.o \
		-o $(ZBIN)/zaf \
		$(ZLIB)/libcurl.a \
	   	$(ZLIB)/libssl.a \
		$(ZLIB)/libcjson.a \
		$(ZLIB)/libcrypto.a  $(LIBS)

zaf.o: zutil.o zsignals.o zservice.o zmodules.o zcommander.o zmemory.o
	gcc $(CFLAGS) -c zaf.c -o$(ZBUILD)/zaf.o -I$(EXTROOT)/curl/include -I$(ZROOT)/include 

zsignals.o: zutil.o
	gcc $(CFLAGS) -c zsignals.c -o$(ZBUILD)/zsignals.o -I$(ZROOT)/include/

zservice.o: zutil.o
	gcc $(CFLAGS) -c zservice.c -o$(ZBUILD)/zservice.o -I$(ZROOT)/include/

zmodules.o: zutil.o zdownloader.o
	gcc $(CFLAGS) -c zmodules.c -o$(ZBUILD)/zmodules.o -I$(ZROOT)/include/  -I$(EXTROOT)/curl/include

zcommander.o: zmodules.o
	gcc $(CFLAGS) -c zcommander.c -o$(ZBUILD)/zcommander.o -I$(ZROOT)/include/  

zdownloader.o:
	gcc $(CFLAGS) -c zdownloader.c -o$(ZBUILD)/zdownloader.o -I$(ZROOT)/include/ -I$(EXTROOT)/curl/include

zmemory.o: zutil.o zmodules.o
	gcc $(CFLAGS) -c zmemory.c -o$(ZBUILD)/zmemory.o -I$(ZROOT)/include/  

zutil.o:
	gcc $(CFLAGS) -c zutil.c -o$(ZBUILD)/zutil.o -I$(ZROOT)/include/



log.o: 
	gcc $(CFLAGS) -c $(ZEXSRC)/log.c -o$(ZBUILD)/log.o -I$(ZROOT)/include/

spt.o:
	gcc $(CFLAGS) -c $(ZEXSRC)/spt_status.c -o $(ZBUILD)/spt.o -I$(ZROOT)/include

clean:
	$(RM) $(ZBUILD)/* $(ZBIN)/*

.PHONY: clean
