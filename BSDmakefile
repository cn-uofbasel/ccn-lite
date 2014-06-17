# BSDMakefile

CC=gcc
#MYCFLAGS=-Wall -pedantic -std=c99 -g
MYCFLAGS=-Wall -g -O0
EXTLIBS=  -lcrypto

NFNFLAGS= -DCCNL_NFN -DCCNL_NFN_MONITOR -DCCNL_NACK

INST_PROGS= ccn-lite-relay \
	    ccn-nfn-relay \
	    ccn-lite-minimalrelay

PROGS=  ${INST_PROGS}

# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	${CCNB_LIB} ${NDNTLV_LIB} BSDmakefile\
	ccnl-core.c ccnl.h ccnl-core.h
	${CC} -o $@ ${MYCFLAGS} $<

ccn-nfn-relay: ccn-lite-relay.c ${CCNB_LIB} ${NDNTLV_LIB} BSDmakefile\
	BSDmakefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c ccnl-ext-nfn.c krivine.c krivine-common.c BSDmakefile
	${CC} -o $@ ${MYCFLAGS} ${NFNFLAGS} $< ${EXTLIBS}

ccn-lite-relay: ccn-lite-relay.c ${CCNB_LIB} ${NDNTLV_LIB} BSDmakefile\
	BSDmakefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c BSDmakefile
	${CC} -o $@ ${MYCFLAGS} $< ${EXTLIBS} -DCCNL_NACK -DCCNL_NFN_MONITOR


datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

install: all
	install ${INST_PROGS} ${INSTALL_PATH}/bin && cd util && make install && cd ..

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null \
	&& cd util && make uninstall && cd ..


clean:
	${EXTMAKECLEAN}
	rm -rf *~ *.o ${PROGS} node-*.log .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	make -C util clean
# eof
