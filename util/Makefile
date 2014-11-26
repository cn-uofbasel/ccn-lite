# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -g -DUSE_UTIL -Wno-error=deprecated-declarations -std=c99 -Werror
EXTLIBS=-lcrypto

PROGS=	ccn-lite-ccnb2xml \
	ccn-lite-cryptoserver\
	ccn-lite-ctrl \
	ccn-lite-mkC \
	ccn-lite-mkI \
	ccn-lite-peek \
	ccn-lite-pktdump \
	ccn-lite-cryptoserver \
	ccn-lite-produce \
	ccn-lite-fetch \
	ccn-lite-simplenfn

#	ccn-lite-peekcomputation \
#	ccn-lite-deF \
#	ccn-lite-mkF \

CCNB_LIB =   ../ccnl-pkt-ccnb.h ../ccnl-pkt-ccnb.c
CCNTLV_LIB = ../ccnl-pkt-ccntlv.h ../ccnl-pkt-ccntlv.c
NDNTLV_LIB = ../ccnl-pkt-ndntlv.h ../ccnl-pkt-ndntlv.c
LOCRPC_LIB = ../ccnl-pkt-localrpc.h ../ccnl-pkt-localrpc.c
SUITE_LIBS = ${CCNB_LIB} ${CCNTLV_LIB} ${LOCALRPC_LIB} ${NDNTLV_LIB}


CCNL_CORE_LIB = ../ccnl-defs.h ../ccnl-core.h ../ccnl-core.c

# CCNL_RELAY_LIB = ../ccn-lite-relay.c ${SUITE_LIBS} \
#                  ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
#                  ../ccnl-ext-mgmt.c ../ccnl-ext-http.c ../ccnl-ext-crypto.c 

CCNL_PLATFORM_LIB = ../ccnl-includes.h \
                    ../ccnl-ext-debug.c ../ccnl-ext.h ../ccnl-platform.c  \
                    ../ccnl-ext-frag.c ../ccnl-ext-sched.c\


NFN_LIB = ccnl-ext-nfn.c krivine.c krivine-common.c

# CCNL_NFN_MONITOR to log messages to nfn-scala montior
NFNFLAGS += -DUSE_NFN -DUSE_NFN_MONITOR
NACKFLAGS += -DUSE_NACK -DUSE_NFN_MONITOR

ifdef USE_NFN
    $(info *** With NFN ***)
    CFLAGS += ${NFNFLAGS}
endif

# ----------------------------------------------------------------------

all: $(PROGS)

ccn-lite-ccnb2xml: Makefile ccn-lite-ccnb2xml.c \
		${CCNB_LIB} ../ccnl-defs.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ccnb2xml.c -o $@ ${EXTLIBS}

# ccn-lite-peek: Makefile ccn-lite-peek.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl.h ../ccnl-core-util.c
# 	$(CC) $(CFLAGS) ccn-lite-peek.c -o $@ 


# ccn-lite-pktdump: Makefile ccn-lite-pktdump.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ${LOCRPC_LIB} ../ccnl.h

ccn-lite-ctrl: Makefile ccn-lite-ctrl.c ${CCNB_LIB} \
		../ccnl-defs.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ctrl.c -o $@ ${EXTLIBS}

ccn-lite-peek: Makefile ccn-lite-peek.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c
	$(CC) $(CFLAGS) ccn-lite-peek.c -o $@ 

ccn-lite-fetch: Makefile ccn-lite-fetch.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-fetch.c -o $@ 

ccn-lite-pktdump: Makefile ccn-lite-pktdump.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ${LOCRPC_LIB}
	$(CC) $(CFLAGS) ccn-lite-pktdump.c -o $@ 

ccn-lite-simplenfn: Makefile ccn-lite-simplenfn.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c
	$(CC) $(CFLAGS) ccn-lite-simplenfn.c -o $@ 

ccn-lite-rpc: Makefile ccn-lite-rpc.c \
		${NDNTLV_LIB} ${LOCRPC_LIB} ../ccnl-defs.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-rpc.c -o $@ 

ccn-lite-deF: Makefile ccn-lite-deF.c ${CCNB_LIB} ../ccnl-defs.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-deF.c -o $@ 

ccn-lite-mkC: Makefile ccn-lite-mkC.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-mkC.c -o $@ ${EXTLIBS}

ccn-lite-mkF: Makefile ccn-lite-mkF.c ${CCNB_LIB} ../ccnl-defs.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-mkF.c -o $@ 

ccn-lite-mkI: Makefile ccn-lite-mkI.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-mkI.c -o $@ 

#ccn-lite-peekcomputation: Makefile ccn-lite-peekcomputation.c ${CCNB_LIB} ../ccnl.h  ccnl-common.c
#	$(CC) $(CFLAGS) ccn-lite-peekcomputation.c -o $@ 

ccn-lite-cryptoserver: Makefile ccn-lite-cryptoserver.c
	$(CC) $(CFLAGS) ccn-lite-cryptoserver.c -o $@ ${EXTLIBS}

ccn-lite-produce: Makefile ccn-lite-produce.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-produce.c -o $@ ${EXTLIBS}

demo: $(PROGS)
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-pktdump
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2xml

install: all
	install ${PROGS} ${INSTALL_PATH}/bin

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null

clean:
	rm -rf *~ $(PROGS)
