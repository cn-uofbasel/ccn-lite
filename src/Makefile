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

# Get version of gcc with two digits, e.g. 40902
GCC_VERSION := $(shell ${CC} -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
GCC_GTEQ_40900 := $(shell expr ${GCC_VERSION} \>= 40900)

# general flags used on both linux and OSX
CCNLCFLAGS=-Wall -Werror -std=c99 -g
	
ifeq ($(RIOTBASE),)
	CCNLCFLAGS += -pedantic -O0
endif

# Linux flags
LINUX_CFLAGS=-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -Dlinux
ifeq ($(GCC_GTEQ_40900),1)
    LINUX_CFLAGS += -Wno-date-time
endif

# OSX, ignore deprecated warnings for libssl
OSX_CFLAGS=-Wno-error=deprecated-declarations

CREATE_BIN=mkdir -p ../bin && cd ../bin && ln -f -s ../src/$@ $@
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
    EXTMAKE=cd ${CHEMFLOW_HOME}; $(MAKE)
    EXTMAKECLEAN=cd ${CHEMFLOW_HOME}; $(MAKE) clean
    CCNLCFLAGS+=-DUSE_CHEMFLOW -I${CHEMFLOW_HOME}/include -L${CHEMFLOW_HOME}/staging/host/lib
endif

EXTRA_CFLAGS := -g $(OPTCFLAGS) -std=gnu99
ifeq ($(GCC_GTEQ_40900),1)
    EXTRA_CFLAGS += -Wno-date-time
endif
obj-m += ccn-lite-lnxkernel.o
#ccn-lite-lnxkernel-objs += ccnl-ext-crypto.o

CCNB_LIB =   ccnl-pkt-ccnb.h ccnl-pkt-ccnb.c lib-sha256.c
CCNTLV_LIB = ccnl-pkt-ccntlv.h ccnl-pkt-ccntlv.c
CISTLV_LIB = ccnl-pkt-cistlv.h ccnl-pkt-cistlv.c
IOTTLV_LIB = ccnl-pkt-iottlv.h ccnl-pkt-iottlv.c
NDNTLV_LIB = ccnl-pkt-ndntlv.h ccnl-pkt-ndntlv.c
LOCALRPC_LIB = ccnl-pkt-localrpc.h ccnl-pkt-localrpc.c ccnl-ext-localrpc.c
SUITE_LIBS = ${CCNB_LIB} ${CCNTLV_LIB} ${CISTLV_LIB} ${IOTTLV_LIB} ${NDNTLV_LIB} ${LOCALRPC_LIB}
UTIL_LIB := util/base64.o util/ccnl-common.o


CCNL_CORE_LIB = ccnl-defs.h ccnl-core.h ccnl-core.c ccnl-core-fwd.c ccnl-core-util.c

CCNL_RELAY_LIB = ccn-lite-relay.c ${SUITE_LIBS} \
                 ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
                 ccnl-ext-echo.c ccnl-ext-hmac.c ccnl-ext-mgmt.c \
		 ccnl-ext-http.c  ccnl-ext-crypto.c

CCNL_PLATFORM_LIB = ccnl-os-includes.h \
                    ccnl-ext-debug.c ccnl-ext.h ccnl-ext-logging.c \
                    ccnl-os-time.c ccnl-ext-frag.c ccnl-ext-sched.c

NFN_LIB = ccnl-ext-nfn.c krivine.c krivine-common.c

OMNET_DEPS = ccnl-core.c ccnl-core-fwd.c ccnl-core.h ccnl-core-util.c ccnl-defs.h \
             ccnl-ext-crypto.c ccnl-ext-debug.c ccnl-ext-debug.h ccnl-ext-echo.c \
             ccnl-ext-frag.c ccnl-ext.h ccnl-ext-hmac.c ccnl-ext-http.c \
             ccnl-ext-localrpc.c ccnl-ext-logging.c ccnl-ext-mgmt.c ccnl-ext-sched.c \
             ccnl-pkt-ccnb.c ccnl-pkt-ccnb.h ccnl-pkt-ccntlv.c ccnl-pkt-ccntlv.h \
             ccnl-pkt-cistlv.c ccnl-pkt-cistlv.h ccnl-pkt-iottlv.c ccnl-pkt-iottlv.h \
             ccnl-pkt-localrpc.c ccnl-pkt-localrpc.h ccnl-pkt-ndntlv.c ccnl-pkt-ndntlv.h \
             ccnl-pkt-switch.c ccnl-uapi.c ccnl-uapi.h lib-sha256.c \
             util/base64.c util/ccnl-common.c \

# ----------------------------------------------------------------------

all: ${PROGS}
	$(MAKE) -C util

lib-ccn-lite.a: ccn-lite-riot.c \
	${SUITE_LIBS} ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB}
	${CC} -o $@ ${RIOT_CFLAGS} ${CCNLCFLAGS} -c $<

lib-ccn-lite-utils.a: ${UTIL_LIB} util/base64.c util/ccnl-common.c
	${AR} rcs $@ ${UTIL_LIB}

$(UTIL_LIB): %.o: %.c
	${CC} -o $@ ${RIOT_CFLAGS} ${CCNLCFLAGS} -c $<

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c \
	${SUITE_LIBS} ${CCNL_CORE_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-nfn-relay: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-nfn-relay-nack: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} ${NACKFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-relay: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-relay-nack: ${CCNL_RELAY_LIB}
	${CC} -o $@ ${CCNLCFLAGS} ${NACKFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-simu: ccn-lite-simu.c  ${SUITE_LIBS} \
	${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} \
	ccnl-simu-client.c
	${EXTMAKE}
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-omnet:  ${OMNET_DEPS}
	rm -rf ccn-lite-omnet
	rm -rf ccn-lite-omnet.tgz
	cp -a omnet ccn-lite-omnet
	mkdir ccn-lite-omnet/src/ccn-lite
	cp -a $^ ccn-lite-omnet/src/ccn-lite/
	tar -zcvf ccn-lite-omnet.tgz ccn-lite-omnet
	rm -rf ccn-lite-omnet

ccn-lite-lnxkernel:
	$(MAKE) modules
#   rm -rf $@.o $@.mod.* .$@* .tmp_versions modules.order Module.symvers

modules: ccn-lite-lnxkernel.c  ${SUITE_LIBS} \
    ${CCNL_CORE_LIB} ${CCNL_PLATFORM_LIB} ccnl-ext-crypto.c
	$(MAKE) -C /lib/modules/`uname -r`/build M=`pwd` V=1 modules

datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

install: all
	install ${INST_PROGS} ${INSTALL_PATH}/bin && cd util && $(MAKE) install && cd ..

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null \
	&& cd util && $(MAKE) uninstall && cd ..

clean:
	${EXTMAKECLEAN}
	rm -rf *~ *.o ${PROGS} ccn-lite-lnxkernel ccn-lite-simu ccn-nfn-relay ccn-lite-relay-nack ccn-nfn-relay-nack
	rm -rf .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	rm -rf ccn-lite-omnet.tgz
	rm -rf ../bin
	$(MAKE) -C util clean
