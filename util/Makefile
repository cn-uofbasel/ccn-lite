# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -Werror -g -DUSE_UTIL -Wno-error=deprecated-declarations
EXTLIBS=-lcrypto

PROGS=	ccn-lite-ctrl \
	ccn-lite-ccnb2xml \
	ccn-lite-mkC \
	ccn-lite-mkI \
	ccn-lite-peek \
	ccn-lite-pktdump \
	ccn-lite-cryptoserver\

#	ccn-lite-peekcomputation \
#	ccn-lite-deF \
#	ccn-lite-mkF \
#	ccn-lite-mkndnD \
#	ccn-lite-mkndnI \

CCNB_LIB =   ../pkt-ccnb.h ../pkt-ccnb-dec.c ../pkt-ccnb-enc.c
CCNTLV_LIB = ../pkt-ccntlv.h ../pkt-ccntlv-dec.c ../pkt-ccntlv-enc.c
NDNTLV_LIB = ../pkt-ndntlv.h ../pkt-ndntlv-dec.c ../pkt-ndntlv-enc.c
LOCRPC_LIB = ../pkt-localrpc.h ../pkt-localrpc-dec.c ../pkt-localrpc-enc.c

all: $(PROGS)

ccn-lite-ctrl: ccn-lite-ctrl.c ${CCNB_LIB} Makefile\
		../ccnl.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ctrl.c -o $@ ${EXTLIBS}

ccn-lite-ccnb2xml: Makefile ccn-lite-ccnb2xml.c ${CCNB_LIB} ../ccnl.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ccnb2xml.c -o $@ ${EXTLIBS}


ccn-lite-peek: Makefile ccn-lite-peek.c ${CCNB_LIB} ${NDNTLV_LIB} ../ccnl.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-peek.c -o $@ 

ccn-lite-pktdump: Makefile ccn-lite-pktdump.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ${LOCRPC_LIB}
	$(CC) $(CFLAGS) ccn-lite-pktdump.c -o $@ 

ccn-lite-rpc: Makefile ccn-lite-rpc.c ${NDNTLV_LIB} ${LOCRPC_LIB} ../ccnl.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-rpc.c -o $@ 

ccn-lite-deF: Makefile ccn-lite-deF.c ${CCNB_LIB} ../ccnl.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-deF.c -o $@ 

ccn-lite-mkC: Makefile ccn-lite-mkC.c ${CCNB_LIB} ../ccnl.h
	$(CC) $(CFLAGS) ccn-lite-mkC.c -o $@ ${EXTLIBS}

ccn-lite-mkF: Makefile ccn-lite-mkF.c ${CCNB_LIB} ../ccnl.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-mkF.c -o $@ 

ccn-lite-mkI: Makefile ccn-lite-mkI.c ../pkt-ccnb.h ../ccnl.h
	$(CC) $(CFLAGS) ccn-lite-mkI.c -o $@ 

#ccn-lite-peekcomputation: Makefile ccn-lite-peekcomputation.c ${CCNB_LIB} ../ccnl.h  ccnl-common.c
#	$(CC) $(CFLAGS) ccn-lite-peekcomputation.c -o $@ 

ccn-lite-mkndnD: Makefile ccn-lite-mkndnD.c ${NDNTLV_LIB}
	$(CC) $(CFLAGS) ccn-lite-mkndnD.c -o $@

ccn-lite-mkndnI: Makefile ccn-lite-mkndnI.c ${NDNTLV_LIB}
	$(CC) $(CFLAGS) ccn-lite-mkndnI.c -o $@

ccn-lite-cryptoserver: Makefile ccn-lite-cryptoserver.c
	$(CC) $(CFLAGS) ccn-lite-cryptoserver.c -o $@ ${EXTLIBS}

demo: $(PROGS)
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-pktdump
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2xml

install: all
	install ${PROGS} ${INSTALL_PATH}/bin

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null

clean:
	rm -rf *~ $(PROGS)
