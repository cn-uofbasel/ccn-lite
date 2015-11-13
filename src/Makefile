# ccn-lite Makefile for Linux and OS X

# Compile time options: variables can either be exported at the
# shell level (% export USE_NFN=1) or be given as parameter to
# make (% make <target> <VAR>=<val>)

#  for compiling the Linux Kernel module, export          USE_KRNL=1
#  for named-function support (NFN), export               USE_NFN=1
#  for NACK support in NFN, export                        USE_NACK=1

MAKEFILE_PATH=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MAKEFILE_PATH)/Variables.mk

all: ${PROGS}
	$(MAKE) -C util

ccn-lite-minimalrelay: ccn-lite-minimalrelay.c ${SUITE_LIBS} ${CCNL_CORE_LIB}
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

ccn-lite-simu: ccn-lite-simu.c  ${SUITE_LIBS} ${CCNL_CORE_LIB} \
		${CCNL_PLATFORM_LIB} ccnl-simu-client.c
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

modules: ccn-lite-lnxkernel.c  ${SUITE_LIBS} ${CCNL_CORE_LIB} \
		${CCNL_PLATFORM_LIB} ccnl-ext-crypto.c
	$(MAKE) -C /lib/modules/`uname -r`/build M=`pwd` V=1 modules

datastruct.pdf: datastruct.dot
	dot -Tps -o datastruct.ps datastruct.dot
	epstopdf datastruct.ps
	rm -f datastruct.ps

install: all
	install ${INST_PROGS} ${INSTALL_PATH}/bin && cd util && +make install && cd ..

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null \
	&& cd util && +make uninstall && cd ..

clean:
	${EXTMAKECLEAN}
	rm -rf *~ *.o ${PROGS} ccn-lite-lnxkernel ccn-lite-simu ccn-nfn-relay ccn-lite-relay-nack ccn-nfn-relay-nack
	rm -rf .ccn-lite-lnxkernel* *.ko *.mod.c *.mod.o .tmp_versions modules.order Module.symvers
	rm -rf ccn-lite-omnet.tgz
	rm -rf ../bin
	$(MAKE) -C util clean
