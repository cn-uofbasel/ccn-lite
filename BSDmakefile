# Makefile

CC=gcc
#MYCFLAGS=-Wall -pedantic -std=c99 -g
MYCFLAGS=-Wall -g -O0

PROGS=	ccn-lite-relay \
	ccn-lite-minimalrelay

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
	ccnl-ext-sched.c ccnl-pdu.c ccnl-ext-encaps.c ccnl-ext-mgmt.c
	${CC} -o $@ ${MYCFLAGS} $<

datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

clean:
	rm -rf *~ *.o ${PROGS} node-*.log .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	make -C util clean

