# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -g -DUSE_UTIL -Wno-error=deprecated-declarations -std=c99 -Werror
EXTLIBS=-lcrypto
CREATE_BIN=mkdir -p ../../bin && cd ../../bin && ln -f -s ../src/util/$@ $@


PROGS=	ccn-lite-ccnb2xml \
	ccn-lite-cryptoserver\
	ccn-lite-ctrl \
	ccn-lite-fetch \
	ccn-lite-mkC \
	ccn-lite-mkI \
	ccn-lite-mkF \
	ccn-lite-peek \
	ccn-lite-pktdump \
	ccn-lite-produce \
	ccn-lite-rpc \
	ccn-lite-simplenfn \
	ccn-lite-valid 
#	ccn-lite-peekcomputation \
#	ccn-lite-deF \

CCNB_LIB =   ../ccnl-pkt-ccnb.h ../ccnl-pkt-ccnb.c ../lib-sha256.c
CCNTLV_LIB = ../ccnl-pkt-ccntlv.h ../ccnl-pkt-ccntlv.c
CISTLV_LIB = ../ccnl-pkt-cisntlv.h ../ccnl-pkt-cistlv.c
IOTTLV_LIB = ../ccnl-pkt-iottlv.h ../ccnl-pkt-iottlv.c
LOCALRPC_LIB = ../ccnl-pkt-localrpc.h ../ccnl-pkt-localrpc.c ../ccnl-ext-localrpc.c
NDNTLV_LIB = ../ccnl-pkt-ndntlv.h ../ccnl-pkt-ndntlv.c

SUITE_LIBS = ${CCNB_LIB} ${CCNTLV_LIB} ${LOCALRPC_LIB} ${NDNTLV_LIB}


CCNL_CORE_LIB = ../ccnl-defs.h ../ccnl-core.h ../ccnl-core.c

# CCNL_RELAY_LIB = ../ccn-lite-relay.c ${SUITE_LIBS} \
#                  ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
#                  ../ccnl-ext-mgmt.c ../ccnl-ext-http.c ../ccnl-ext-crypto.c

CCNL_PLATFORM_LIB = ../ccnl-includes.h \
                    ../ccnl-ext-debug.c ../ccnl-ext.h ../ccnl-platform.c  \
                    ../ccnl-ext-frag.c ../ccnl-ext-sched.c ../ccnl-ext-hmac.c \
                    ../ccnl-ext-logging.c


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
	@${CREATE_BIN}

ccn-lite-ctrl: Makefile ccn-lite-ctrl.c ${CCNB_LIB} \
		../ccnl-defs.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-ctrl.c -o $@ ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-peek: Makefile ccn-lite-peek.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c
	$(CC) $(CFLAGS) ccn-lite-peek.c -o $@
	@${CREATE_BIN}

ccn-lite-fetch: Makefile ccn-lite-fetch.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-fetch.c -o $@
	@${CREATE_BIN}

ccn-lite-pktdump: Makefile ccn-lite-pktdump.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${IOTTLV_LIB} ${NDNTLV_LIB} \
		${LOCALRPC_LIB}
	$(CC) $(CFLAGS) ccn-lite-pktdump.c -o $@
	@${CREATE_BIN}

ccn-lite-simplenfn: Makefile ccn-lite-simplenfn.c \
		${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c
	$(CC) $(CFLAGS) ccn-lite-simplenfn.c -o $@
	@${CREATE_BIN}

ccn-lite-rpc: Makefile ccn-lite-rpc.c \
		${NDNTLV_LIB} ${LOCALRPC_LIB} ../ccnl-defs.h ccnl-common.c
	$(CC) $(CFLAGS) ccn-lite-rpc.c -o $@
	@${CREATE_BIN}

ccn-lite-deF: Makefile ccn-lite-deF.c ${CCNB_LIB} ../ccnl-defs.h ccnl-frag.h
	$(CC) $(CFLAGS) ccn-lite-deF.c -o $@
	@${CREATE_BIN}

ccn-lite-mkC: Makefile ccn-lite-mkC.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-mkC.c -o $@ ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-mkF: Makefile ccn-lite-mkF.c ${CCNB_LIB} ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-mkF.c -o $@
	@${CREATE_BIN}

ccn-lite-mkI: Makefile ccn-lite-mkI.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-mkI.c -o $@
	@${CREATE_BIN}

#ccn-lite-peekcomputation: Makefile ccn-lite-peekcomputation.c ${CCNB_LIB} ../ccnl.h  ccnl-common.c
#	$(CC) $(CFLAGS) ccn-lite-peekcomputation.c -o $@
#	@${CREATE_BIN}

ccn-lite-cryptoserver: Makefile ccn-lite-cryptoserver.c
	$(CC) $(CFLAGS) ccn-lite-cryptoserver.c -o $@ ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-produce: Makefile ccn-lite-produce.c ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} ../ccnl-defs.h
	$(CC) $(CFLAGS) ccn-lite-produce.c -o $@ ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-valid: Makefile ccn-lite-valid.c ccnl-common.c \
		${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c
	$(CC) $(CFLAGS) ccn-lite-valid.c -o $@
	@${CREATE_BIN}

demo: $(PROGS)
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-pktdump
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2xml

install: all
	install ${PROGS} ${INSTALL_PATH}/bin

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null

clean:
	rm -rf *~ $(PROGS)
	rm -rf ../../bin
