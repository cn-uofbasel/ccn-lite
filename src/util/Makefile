# ccnl/util/Makefile

MAKEFILE_PATH=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(MAKEFILE_PATH)/../Variables.mk

CCNLCFLAGS+=-DUSE_UTIL
CREATE_BIN=mkdir -p ../../bin && cd ../../bin && ln -f -s ../src/util/$@ $@


PROGS:=ccn-lite-ccnb2xml \
       ccn-lite-cryptoserver\
       ccn-lite-ctrl \
       ccn-lite-fetch \
       ccn-lite-flic \
       ccn-lite-mkC \
       ccn-lite-mkF \
       ccn-lite-mkI \
       ccn-lite-mkM \
       ccn-lite-peek \
       ccn-lite-pktdump \
       ccn-lite-produce \
       ccn-lite-rpc \
       ccn-lite-simplenfn \
       ccn-lite-valid

#       ccn-lite-peekcomputation \
#       ccn-lite-deF \

CCNB_LIB :=     $(addprefix ../,${CCNB_LIB})
CCNTLV_LIB :=   $(addprefix ../,${CCNTLV_LIB})
CISTLV_LIB :=   $(addprefix ../,${CISTLV_LIB})
IOTTLV_LIB :=   $(addprefix ../,${IOTTLV_LIB})
LOCALRPC_LIB := $(addprefix ../,${LOCALRPC_LIB})
NDNTLV_LIB :=   $(addprefix ../,${NDNTLV_LIB})
SUITE_LIBS :=   $(addprefix ../,${SUITE_LIBS})

CCNL_CORE_LIB :=     $(addprefix ../,${CCNL_CORE_LIB})
CCNL_RELAY_LIB :=    $(addprefix ../,${CCNL_RELAY_LIB})
CCNL_PLATFORM_LIB := $(addprefix ../,${CCNL_PLATFORM_LIB})

NFN_LIB := $(addprefix ../,${NFN_LIB})

# ----------------------------------------------------------------------

all: $(PROGS)

ccn-lite-ccnb2xml: ccn-lite-ccnb2xml.c Makefile ${CCNB_LIB} ../ccnl-defs.h \
		ccnl-common.c
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-ctrl: ccn-lite-ctrl.c Makefile ${CCNB_LIB} ../ccnl-defs.h \
		ccnl-common.c
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-peek: ccn-lite-peek.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h ../ccnl-core-util.c ../ccnl-core-pfx.c
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-fetch: ccn-lite-fetch.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h ../ccnl-core-util.c ../ccnl-core-pfx.c ccnl-common.c
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-flic: ccn-lite-flic.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${IOTTLV_LIB} ${NDNTLV_LIB} ${LOCALRPC_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-pktdump: ccn-lite-pktdump.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${IOTTLV_LIB} ${NDNTLV_LIB} ${LOCALRPC_LIB}
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-simplenfn: ccn-lite-simplenfn.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h ../ccnl-core-util.c ../ccnl-core-pfx.c
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-rpc: ccn-lite-rpc.c Makefile ${NDNTLV_LIB} ${LOCALRPC_LIB} \
		../ccnl-defs.h ccnl-common.c
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-deF: ccn-lite-deF.c Makefile ${CCNB_LIB} ../ccnl-defs.h ccnl-frag.h
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-mkC: ccn-lite-mkC.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-mkF: ccn-lite-mkF.c Makefile ${CCNB_LIB} ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-mkI: ccn-lite-mkI.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

ccn-lite-mkM: ccn-lite-mkM.c Makefile ${CCNB_LIB} ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

#ccn-lite-peekcomputation: ccn-lite-peekcomputation.c Makefile ${CCNB_LIB} \
#		../ccnl.h  ccnl-common.c
#	${CC} -o $@ ${CCNLCFLAGS} $<
#	@${CREATE_BIN}

ccn-lite-cryptoserver: ccn-lite-cryptoserver.c Makefile
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-produce: ccn-lite-produce.c Makefile ${CCNB_LIB} ${CCNTLV_LIB} \
		${NDNTLV_LIB} ../ccnl-defs.h
	${CC} -o $@ ${CCNLCFLAGS} $< ${EXTLIBS}
	@${CREATE_BIN}

ccn-lite-valid: ccn-lite-valid.c Makefile ccnl-common.c \
		${CCNTLV_LIB} ${NDNTLV_LIB} \
		../ccnl-defs.h ../ccnl-core-util.c ../ccnl-core-pfx.c
	${CC} -o $@ ${CCNLCFLAGS} $<
	@${CREATE_BIN}

demo: $(PROGS)
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-pktdump
	./ccn-lite-mkI /ccn-lite/says/hello/world/äöü | ./ccn-lite-ccnb2xml

install: all
	install ${PROGS} ${INSTALL_PATH}/bin

uninstall:
	cd ${INSTALL_PATH}/bin && rm -f ${PROGS} && cd - > /dev/null

clean:
	rm -rf *~ $(PROGS)
	rm -rf ../../bin
