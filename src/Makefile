# ccn-lite Makefile for Linux and OS X

# Compile time options: variables can either be exported at the
# shell level (% export USE_NFN=1) or be given as parameter to
# make (% make <target> <VAR>=<val>)

#  for compiling the Linux Kernel module, export          USE_KRNL=1
#  for named-function support (NFN), export               USE_NFN=1
#  for NACK support in NFN, export                        USE_NACK=1


# OS name: Linux or Darwing (OSX) supported
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CC?=gcc

# general flags used on both linux and OSX
CCNLCFLAGS=-Wall -Werror -pedantic -std=c99 -g -O0 
# CCNLCFLAGS=-Wall -pedantic -std=c99 -g -O0

# Linux flags
LINUX_CFLAGS=-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -Dlinux -Wno-error=date-time

# OSX, ignore deprecated warnings for libssl
OSX_CFLAGS=-Wno-error=deprecated-declarations

EXTLIBS=  -lcrypto 
EXTMAKE=
EXTMAKECLEAN=

# CCNL_NFN_MONITOR to log messages to nfn-scala montior
NFNFLAGS += -DUSE_NFN #-DUSE_NFN_MONITOR
NACKFLAGS += -DUSE_NACK -DUSE_NFN_MONITOR

PROGS = 
INST_PROGS= ccn-lite-relay \
            ccn-lite-minimalrelay 

# Linux specific (adds kernel module)
ifeq ($(uname_S),Linux)
    $(info *** Configuring for Linux ***)
    EXTLIBS += -lrt
    ifdef USE_KRNL
        $(info *** With Linux Kernel ***)
        PROGS += ccn-lite-lnxkernel 
    endif

    # Some investigation needed to compile ccn-lite-simu with OSX
    PROGS += ccn-lite-simu
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
    CCNLCFLAGS += ${NFNFLAGS}
endif

ifdef USE_NACK
    $(info *** With NFN_NACK ***)
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

EXTRA_CFLAGS := -g $(OPTCFLAGS) -Wno-error=date-time
obj-m += ccn-lite-lnxkernel.o
#ccn-lite-lnxkernel-objs += ccnl-ext-crypto.o

CCNB_LIB =   ccnl-pkt-ccnb.h ccnl-pkt-ccnb.c lib-sha256.c
CCNTLV_LIB = ccnl-pkt-ccntlv.h ccnl-pkt-ccntlv.c
CISTLV_LIB = ccnl-pkt-cistlv.h ccnl-pkt-cistlv.c
IOTTLV_LIB = ccnl-pkt-iottlv.h ccnl-pkt-iottlv.c
NDNTLV_LIB = ccnl-pkt-ndntlv.h ccnl-pkt-ndntlv.c
LOCALRPC_LIB = ccnl-pkt-localrpc.h ccnl-pkt-localrpc.c ccnl-ext-localrpc.c
SUITE_LIBS = ${CCNB_LIB} ${CCNTLV_LIB} ${CISTLV_LIB} ${IOTTLV_LIB} ${NDNTLV_LIB} ${LOCALRPC_LIB}


CCNL_CORE_LIB = ccnl-defs.h ccnl-core.h ccnl-core.c ccnl-core-fwd.c ccnl-core-util.c

CCNL_RELAY_LIB = ccn-lite-relay.c ${SUITE_LIBS} \
                 ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
                 ccnl-ext-echo.c ccnl-ext-hmac.c ccnl-ext-mgmt.c \
		 ccnl-ext-http.c  ccnl-ext-crypto.c 

CCNL_PLATFORM_LIB = ccnl-os-includes.h \
                    ccnl-ext-debug.c ccnl-ext.h ccnl-ext-logging.c \
                    ccnl-os-time.c ccnl-ext-frag.c ccnl-ext-sched.c

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
	ccnl-simu-client.c
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
	make -C /lib/modules/`uname -r`/build M=`pwd` V=1 modules

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
	rm -rf *~ *.o ${PROGS}
	rm -rf .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	rm -rf omnet/src/ccn-lite/*
	rm -rf ccn-lite-omnet.tgz
	make -C util clean
