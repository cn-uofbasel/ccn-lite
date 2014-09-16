# Makefile
# Makefile for both Linux and OS X
# If nfn targets should be compiled as well set USE_NFN environment variable to something 
# or add commandline option -e <FLAG>=0
# For (experimental) nack set USE_NACKS to something

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CC?=gcc
CFLAGS=-O3 -Wall -Werror -pedantic -std=c99 -g
LINUX_CFLAGS=-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -Dlinux -O0
OSX_CFLAGS=-Wno-error=deprecated-declarations

EXTLIBS=  -lcrypto 
EXTMAKE=
EXTMAKECLEAN=
NFNFLAGS += -DCCNL_NFN -DCCNL_NFN_MONITOR

INST_PROGS= ccn-lite-relay \
            ccn-lite-minimalrelay 

ifeq ($(uname_S),Linux)
    $(info *** Configuring for Linux (with kernel module) ***)
    EXTLIBS += -lrt
    INST_PROGS += ccn-lite-lnxkernel \
                  ccn-lite-simu
    CFLAGS += ${LINUX_CFLAGS}
endif

ifeq ($(uname_S),Darwin)
    $(info *** Configuring for OSX ***)
    CFLAGS += ${OSX_CFLAGS}
    EXTMAKECLEAN += rm -rf *.dSYM
endif

ifdef USE_NFN
    $(info *** With NFN ***)
    INST_PROGS += ccn-nfn-relay
endif

ifdef USE_NACKS
    $(info *** With NFN_NACKS ***)
    INST_PROGS += ccn-lite-relay-nack 
    ifdef USE_NFN
        INST_PROGS += ccn-nfn-relay-nack
    endif
endif

PROGS=  ${INST_PROGS}

ifdef USE_CHEMFLOW
CHEMFLOW_HOME=./chemflow/chemflow-20121006
EXTLIBS=-lcf -lcfserver -lcrypto
EXTMAKE=cd ${CHEMFLOW_HOME}; make
EXTMAKECLEAN=cd ${CHEMFLOW_HOME}; make clean
CFLAGS+=-DUSE_CHEMFLOW -I${CHEMFLOW_HOME}/include -L${CHEMFLOW_HOME}/staging/host/lib
endif

EXTRA_CFLAGS := -Wall -g $(OPTCFLAGS)
obj-m += ccn-lite-lnxkernel.o
#ccn-lite-lnxkernel-objs += ccnl-ext-crypto.o

CCNB_LIB =      pkt-ccnb.h pkt-ccnb-dec.c pkt-ccnb-enc.c fwd-ccnb.c
NDNTLV_LIB =    pkt-ndntlv.h pkt-ndntlv-dec.c pkt-ndntlv-enc.c fwd-ndntlv.c
LOCRPC_LIB =    pkt-localrpc.h pkt-localrpc-enc.c pkt-localrpc-dec.c fwd-localrpc.c

# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} Makefile\
	ccnl-core.c ccnl.h ccnl-core.h
	${CC} -o $@ ${CFLAGS} $<

ccn-nfn-relay: ccn-lite-relay.c ${CCNB_LIB} ${NDNTLV_LIB} Makefile\
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c ccnl-ext-nfn.c krivine.c krivine-common.c Makefile
	${CC} -o $@ ${CFLAGS} ${NFNFLAGS} $< ${EXTLIBS}

ccn-nfn-relay-nack: ccn-lite-relay.c ${CCNB_LIB} ${NDNTLV_LIB} Makefile\
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c ccnl-ext-nfn.c krivine.c krivine-common.c Makefile
	${CC} -o $@ ${CFLAGS} ${NFNFLAGS} -DCCNL_NACK $< ${EXTLIBS}

ccn-lite-relay: ccn-lite-relay.c ${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c Makefile
	${CC} -o $@ ${CFLAGS} $< ${EXTLIBS} 

ccn-lite-relay-nack: ccn-lite-relay.c ${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-http.c \
	ccnl-ext-sched.c ccnl-ext-frag.c ccnl-ext-mgmt.c \
	ccnl-ext-crypto.c Makefile
	${CC} -o $@ ${CFLAGS} -DCCNL_NACK -DCCNL_NFN_MONITOR $< ${EXTLIBS} 

ccn-lite-simu: ccn-lite-simu.c  ${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-frag.c ccnl-ext-sched.c ccnl-simu-client.c ccnl-util.c
	${EXTMAKE}
	${CC} -o $@ ${CFLAGS} $< ${EXTLIBS}

ccn-lite-omnet:  ${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} Makefile \
	ccnl-core.c ccnl-core.h ccnl-ext-debug.c \
	ccnl-ext-frag.c ccnl-ext.h ccnl-ext-sched.c ccnl.h \
	ccnl-includes.h ccn-lite-omnet.c ccnl-platform.c \
	Makefile
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	cp -a $^ omnet/src/ccn-lite/
	mv omnet ccn-lite-omnet
	tar -zcvf ccn-lite-omnet.tgz ccn-lite-omnet
	mv ccn-lite-omnet omnet

ccn-lite-lnxkernel:
	make modules
#   rm -rf $@.o $@.mod.* .$@* .tmp_versions modules.order Module.symvers

modules: ccn-lite-lnxkernel.c  ${CCNB_LIB} ${LOCRPC_LIB} ${NDNTLV_LIB} \
	Makefile ccnl-includes.h ccnl.h ccnl-core.h \
	ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c ccnl-core.c \
	ccnl-ext-frag.c ccnl-ext-sched.c ccnl-ext-crypto.c Makefile
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
