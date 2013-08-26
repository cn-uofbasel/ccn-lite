# BSDMakefile

CC=gcc
#MYCFLAGS=-Wall -pedantic -std=c99 -g
MYCFLAGS=-Wall -g -O0
EXTLIBS=  -lcrypto

INST_PROGS= ccn-lite-relay \
            ccn-lite-minimalrelay

PROGS=	${INST_PROGS}

# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	BSDmakefile ccnl-core.c ccnx.h ccnl.h ccnl-core.h
	${CC} -o $@ ${MYCFLAGS} $<

ccn-lite-relay: ccn-lite-relay.c \
	BSDmakefile ccnl-includes.h ccnx.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-pdu.c ccnl-ext-frag.c ccnl-ext-mgmt.c
	${CC} -o $@ ${MYCFLAGS} $< ${EXTLIBS}

datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

install: all
	install ${INST_PROGS} ${INSTALL_PATH}/bin \
	&& cd util && make install && cd ..

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
