# Makefile

CC=gcc
#MYCFLAGS=-Wall -pedantic -std=c99 -g
MYCFLAGS=-Wall -g -O0
EXTLIBS=
EXTMAKE=
EXTMAKECLEAN=

PROGS=	ccn-lite-relay \
	ccn-lite-minimalrelay \
	ccn-lite-simu \
	ccn-lite-lnxkernel

ifdef USE_CHEMFLOW
CHEMFLOW_HOME=./chemflow/chemflow-20121006
EXTLIBS=-lcf -lcfserver
EXTMAKE=cd ${CHEMFLOW_HOME}; make
EXTMAKECLEAN=cd ${CHEMFLOW_HOME}; make clean
MYCFLAGS+=-DUSE_CHEMFLOW -I${CHEMFLOW_HOME}/include -L${CHEMFLOW_HOME}/staging/host/lib
endif

EXTRA_CFLAGS := -Wall -g 
obj-m += ccn-lite-lnxkernel.o

# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	Makefile ccnl-core.c ccnx.h ccnl.h ccnl-core.h
	${CC} -o $@ ${MYCFLAGS} $<

ccn-lite-relay: ccn-lite-relay.c \
	Makefile ccnl-includes.h ccnx.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-platform.c ccnl-core.c \
	ccnl-ext-sched.c ccnl-pdu.c ccnl-ext-encaps.c ccnl-ext-mgmt.c
	${CC} -o $@ ${MYCFLAGS} $<

ccn-lite-simu: ccn-lite-simu.c \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-platform.c ccnl-core.c \
	ccnl-ext-encaps.c ccnl-pdu.c ccnl-ext-sched.c ccnl-simu-client.c
	${EXTMAKE}
	${CC} -o $@ ${MYCFLAGS} $< ${EXTLIBS}

ccn-lite-lnxkernel:
	make modules
#	rm -rf $@.o $@.mod.* .$@* .tmp_versions modules.order Module.symvers

modules: ccn-lite-lnxkernel.c ccnl.h ccnl-core.h ccnl-ext-debug.c \
	 ccnl-core.c ccnl-pdu.c ccnl-ext-encaps.c ccnl-ext-sched.c
	make -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) modules

datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

clean:
	${EXTMAKECLEAN}
	rm -rf *~ *.o ${PROGS} node-*.log .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	make -C util clean
