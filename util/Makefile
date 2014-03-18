# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -g
EXTLIBS=-lcrypto

PROGS=	ccn-lite-ctrl \
	ccn-lite-ccnb2hex \
	ccn-lite-ccnb2xml \
	ccn-lite-ccntlv2hex \
	ccn-lite-ndntlv2hex \
	ccn-lite-deF \
	ccn-lite-mkC \
	ccn-lite-mkF \
	ccn-lite-mkI \
	ccn-lite-peek \
	ccn-lite-cryptoserver

#	ccn-lite-mkndnD \
#	ccn-lite-mkndnI \

all: $(PROGS)

ccn-lite-ctrl: Makefile ccn-lite-ctrl.c ../pkt-ccnb.h ../ccnl.h ../pkt-ccnb.c ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ctrl.c -o $@ ${EXTLIBS}

ccn-lite-ccnb2hex: Makefile ccn-lite-ccnb2hex.c ../pkt-ccnb.h ../ccnl.h
	$(CC) $(CFLAGS) ccn-lite-ccnb2hex.c -o $@ 

ccn-lite-ccnb2xml: Makefile ccn-lite-ccnb2xml.c ../pkt-ccnb.h ../ccnl.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ccnb2xml.c -o $@ ${EXTLIBS}

ccn-lite-ccntlv2hex: Makefile ccn-lite-ccntlv2hex.c ../pkt-ccntlv.c ../pkt-ccntlv.h
	$(CC) $(CFLAGS) ccn-lite-ccntlv2hex.c -o $@ 

ccn-lite-ndntlv2hex: Makefile ccn-lite-ndntlv2hex.c ../pkt-ndntlv.c ../pkt-ndntlv.h
	$(CC) $(CFLAGS) ccn-lite-ndntlv2hex.c -o $@ 

ccn-lite-peek: Makefile ccn-lite-peek.c ../pkt-ccnb.h ../ccnl.h ../pkt-ccnb.c ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-peek.c -o $@ 

ccn-lite-deF: Makefile ccn-lite-deF.c ../pkt-ccnb.h ../ccnl.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-deF.c -o $@ 

ccn-lite-mkC: Makefile ccn-lite-mkC.c ../pkt-ccnb.h ../ccnl.h
	$(CC) $(CFLAGS) ccn-lite-mkC.c -o $@ ${EXTLIBS}

ccn-lite-mkF: Makefile ccn-lite-mkF.c ../pkt-ccnb.h ../ccnl.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-mkF.c -o $@ 

ccn-lite-mkI: Makefile ccn-lite-mkI.c ../pkt-ccnb.h ../ccnl.h
	$(CC) $(CFLAGS) ccn-lite-mkI.c -o $@ 

ccn-lite-mkndnD: Makefile ccn-lite-mkndnD.c ../pkt-ndntlv.c ../pkt-ndntlv.h
	$(CC) $(CFLAGS) ccn-lite-mkndnD.c -o $@

ccn-lite-mkndnI: Makefile ccn-lite-mkndnI.c ../pkt-ndntlv.c ../pkt-ndntlv.h
	$(CC) $(CFLAGS) ccn-lite-mkndnI.c -o $@

ccn-lite-cryptoserver: Makefile ccn-lite-cryptoserver.c
	$(CC) $(CFLAGS) ccn-lite-cryptoserver.c -o $@ ${EXTLIBS}

demo: $(PROGS)
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2hex
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2xml

install: all
	install ${PROGS} ${INSTALL_PATH}/bin

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null

clean:
	rm -rf *~ $(PROGS)
