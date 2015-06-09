# build-test.mk

PROFILES=bt-relay-nothing \
	bt-relay-barebones \
	bt-relay-vanilla \
	bt-relay-frag \
	bt-relay-authCtrl \
	bt-relay-nfn \
	bt-relay-all \
	bt-lnx-kernel \
	bt-all

.PHONY: all ${PROFILES}
all: ${PROFILES}

bt-relay-nothing:
# Build without any USE_*
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	UNSET_VARS="USE_CCNxDIGEST USE_DEBUG USE_DEBUG_MALLOC USE_ECHO \
		USE_ETHERNET USE_HMAC256 USE_HTTP_STATUS USE_IPV4 USE_MGMT \
		USE_NACK USE_NFN USE_NFN_NSTRANS USE_NFN_MONITOR USE_SCHEDULER \
		USE_STATS USE_SUITE_CCNB USE_SUITE_CCNTLV USE_SUITE_CISTLV \
		USE_SUITE_IOTTLV USE_SUITE_NDNTLV USE_SUITE_LOCALRPC USE_UNIXSOCKET" \
	./build-test-helper.sh

bt-relay-barebones:
# Build only with USE_IPV4 and USE_SUITE_NDNTLV
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	SET_VARS="USE_IPV4 USE_SUITE_NDNTLV" \
	UNSET_VARS="USE_CCNxDIGEST USE_DEBUG USE_DEBUG_MALLOC USE_ECHO \
		USE_ETHERNET USE_HMAC256 USE_HTTP_STATUS USE_MGMT \
		USE_NACK USE_NFN USE_NFN_NSTRANS USE_NFN_MONITOR USE_SCHEDULER \
		USE_STATS USE_SUITE_CCNB USE_SUITE_CCNTLV USE_SUITE_CISTLV \
		USE_SUITE_IOTTLV USE_SUITE_LOCALRPC USE_UNIXSOCKET" \
	./build-test-helper.sh

bt-relay-vanilla:
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	./build-test-helper.sh

bt-relay-frag:
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	MAKE_VARS="USE_FRAG=1" \
	./build-test-helper.sh

bt-relay-authCtrl:
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	MAKE_VARS="USE_SIGNATURES=1" \
	./build-test-helper.sh

bt-relay-nfn:
	@MAKE_TARGETS="clean ccn-nfn-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	MAKE_VARS="USE_NFN=1" \
	./build-test-helper.sh

bt-relay-all:
	@MAKE_TARGETS="clean ccn-lite-relay" \
	LOG_FNAME=$@ \
	TARGET_FNAME=ccn-lite-relay.c \
	SET_VARS="USE_FRAG" \
	./build-test-helper.sh

bt-lnx-kernel:
	@MAKE_TARGETS="lnx-kernel" \
	LOG_FNAME=$@ \
	./build-test-helper.sh
bt-all:
	@MAKE_TARGETS="all" \
	LOG_FNAME=$@ \
	./build-test-helper.sh
