# Makefile

CC=gcc
MYCFLAGS=-O3 -Wall -pedantic -std=c99 -g \
	-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -Dlinux -O0
#MYCFLAGS= -Wall -g -O0
EXTLIBS=  -lcrypto
EXTMAKE=
EXTMAKECLEAN=

INST_PROGS= ccn-lite-relay \
            ccn-lite-minimalrelay \
            ccn-lite-simu

PROGS=	${INST_PROGS} \
	ccn-lite-lnxkernel



ifdef USE_CHEMFLOW
CHEMFLOW_HOME=./chemflow/chemflow-20121006
EXTLIBS=-lcf -lcfserver -lcrypto
EXTMAKE=cd ${CHEMFLOW_HOME}; make
EXTMAKECLEAN=cd ${CHEMFLOW_HOME}; make clean
MYCFLAGS+=-DUSE_CHEMFLOW -I${CHEMFLOW_HOME}/include -L${CHEMFLOW_HOME}/staging/host/lib
endif

EXTRA_CFLAGS := -Wall -g $(OPTCFLAGS)
obj-m += ccn-lite-lnxkernel.o
#ccn-lite-lnxkernel-objs += ccnl-ext-crypto.o

# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	Makefile ccnl-core.c ccnx.h ccnl.h ccnl-core.h Makefile
	${CC} -o $@ ${MYCFLAGS} $<

ccn-lite-relay: ccn-lite-relay.c \
	Makefile ccnl-includes.h ccnx.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-pdu.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c Makefile
	${CC} -o $@ ${MYCFLAGS} $< ${EXTLIBS}

ccn-lite-simu: ccn-lite-simu.c \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-frag.c ccnl-pdu.c ccnl-ext-sched.c ccnl-simu-client.c Makefile
	${EXTMAKE}
	${CC} -o $@ ${MYCFLAGS} $< ${EXTLIBS}

ccn-lite-omnet: ccnl-core.c ccnl-core.h ccnl-ext-debug.c \
	ccnl-ext-frag.c ccnl-ext.h ccnl-ext-sched.c ccnl.h \
	ccnl-includes.h ccn-lite-omnet.c ccnl-pdu.c ccnl-platform.c \
	ccnx.h Makefile
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	cp -a $^ omnet/src/ccn-lite/
	mv omnet ccn-lite-omnet
	tar -zcvf ccn-lite-omnet.tgz ccn-lite-omnet
	mv ccn-lite-omnet omnet

ccn-lite-lnxkernel:
	make modules
#	rm -rf $@.o $@.mod.* .$@* .tmp_versions modules.order Module.symvers

modules: ccn-lite-lnxkernel.c \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-frag.c ccnl-pdu.c ccnl-ext-sched.c ccnl-ext-crypto.c Makefile
	make -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) modules

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

