# ccn-lite Makefile for Linux and OS X

# If nfn targets should be compiled export USE_NFN environment variable to something 
# or add commandline option -e <FLAG>=0 to make
# For (experimental) nack set USE_NACKS to something

# OS name: Linux or Darwing (OSX) supported
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CC?=gcc

# general flags used on both linux and OSX
CCNLCFLAGS=-O3 -Wall -Werror -pedantic -std=c99 -g


# Linux flags with support to compile kernel module
LINUX_CFLAGS=-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -Dlinux -O0

# OSX, ignore deprecated warnings for libssl
OSX_CFLAGS=-Wno-error=deprecated-declarations

EXTLIBS=  -lcrypto 
EXTMAKE=
EXTMAKECLEAN=

# CCNL_NFN_MONITOR to log messages to nfn-scala montior
NFNFLAGS += -DCCNL_NFN -DCCNL_NFN_MONITOR
NACKFLAGS += -DCCNL_NACK -DCCNL_NFN_MONITOR

INST_PROGS= ccn-lite-relay \
            ccn-lite-minimalrelay 

# Linux specific (adds kernel module)
ifeq ($(uname_S),Linux)
    $(info *** Configuring for Linux (with kernel module) ***)
    EXTLIBS += -lrt
    INST_PROGS += ccn-lite-simu
    PROGS += ccn-lite-lnxkernel 
    CCNLCFLAGS += ${LINUX_CFLAGS}
endif

# OSX specific
ifeq ($(uname_S),Darwin)
    $(info *** Configuring for OSX ***)
    CCNLCFLAGS += ${OSX_CFLAGS}

    # removes debug compile artifacts
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

PROGS += ${INST_PROGS}

ifdef USE_CHEMFLOW
    CHEMFLOW_HOME=./chemflow/chemflow-20121006
    EXTLIBS=-lcf -lcfserver -lcrypto
    EXTMAKE=cd ${CHEMFLOW_HOME}; make
    EXTMAKECLEAN=cd ${CHEMFLOW_HOME}; make clean
    CCNLCFLAGS+=-DUSE_CHEMFLOW -I${CHEMFLOW_HOME}/include -L${CHEMFLOW_HOME}/staging/host/lib
endif

EXTRA_CFLAGS := -Wall -g $(OPTCFLAGS)
obj-m += ccn-lite-lnxkernel.o
#ccn-lite-lnxkernel-objs += ccnl-ext-crypto.o

CCNB_LIB =   pkt-ccnb.h pkt-ccnb-dec.c pkt-ccnb-enc.c fwd-ccnb.c
NDNTLV_LIB = pkt-ndntlv.h pkt-ndntlv-dec.c pkt-ndntlv-enc.c fwd-ndntlv.c
LOCRPC_LIB = pkt-localrpc.h pkt-localrpc-enc.c pkt-localrpc-dec.c fwd-localrpc.c
SUITE_LIBS = ${CCNB_LIB} ${LOCALRPC_LIB} ${NDNTLV_LIB}


CCNL_CORE_LIB = ccnl.h ccnl-core.h ccnl-core.c

CCNL_RELAY_LIB = ccn-lite-relay.c ${SUITE_LIBS} \
                 ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
                 ccnl-ext-mgmt.c ccnl-ext-http.c ccnl-ext-crypto.c 

CCNL_PLATFORM_LIB = ccnl-includes.h \
                    ccnl-ext-debug.c ccnl-ext.h ccnl-platform.c  \
                    ccnl-ext-frag.c ccnl-ext-sched.c\


NFN_LIB = ccnl-ext-nfn.c krivine.c krivine-common.c


# ----------------------------------------------------------------------

all: ${PROGS}
	make -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	${SUITE_LIBS} ${CCNL_CORE_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $<

ccn-nfn-relay: ${CCNL_RELAY_LIB} 
	${CC} -o $@ ${CCNLCFLAGS} ${NFNFLAGS} $< ${EXTLIBS}

ccn-nfn-relay-nack: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} ${NFNFLAGS} ${NACKFLAGS} $< ${EXTLIBS}

ccn-lite-relay: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS} 

ccn-lite-relay-nack: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} ${NACKFLAGS} $< ${EXTLIBS} 

ccn-lite-simu: ccn-lite-simu.c  ${SUITE_LIBS} \
	${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
	ccnl-simu-client.c ccnl-util.c
	${EXTMAKE}
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}

ccn-lite-omnet:  ${SUITE_LIBS} \
	${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
	ccn-lite-omnet.c 
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	cp -a $^ omnet/src/ccn-lite/
	mv omnet ccn-lite-omnet
	tar -zcvf ccn-lite-omnet.tgz ccn-lite-omnet
	mv ccn-lite-omnet omnet

ccn-lite-lnxkernel:
	make modules
#   rm -rf $@.o $@.mod.* .$@* .tmp_versions modules.order Module.symvers

modules: ccn-lite-lnxkernel.c  ${SUITE_LIBS} \
    ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} ccnl-ext-crypto.c 
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
