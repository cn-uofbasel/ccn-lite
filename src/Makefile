

USE_FLAGS = 				\
    -DUSE_LOGGING 			\
    -DUSE_CCNxDIGEST 		\
    -DUSE_DUP_CHECK 		\
    -DUSE_HMAC256 			\
    -DUSE_IPV4 				\
    -DUSE_IPV6 				\
    -DUSE_STATS 			\
    -DUSE_UNIXSOCKET 		\
    -DNEEDS_PACKET_CRAFTING \
    -DNEEDS_PREFIX_MATCHING \
#    -DUSE_MGMT 			\
#    -DUSE_ECHO 			\


PLATFORM_FLAGS = -DCCNL_UNIX 	# -DCCNL_ARDUINO

PACKET_FORMATS = 			\
    -DUSE_SUITE_NDNTLV		\
    -DUSE_SUITE_CCNB		\
    -DUSE_SUITE_CISTLV		\
    -DUSE_SUITE_IOTTLV		\
    -DUSE_SUITE_LOCALRPC

# NFN_FLAGS = -DUSE_NFN # -DUSE_NFN_NSTRANS

CCNL_FLAGS = $(USE_FLAGS) $(PLATFORM_FLAGS) $(PACKET_FORMATS) $(NFN_FLAGS)
CFLAGS = -Wall -Werror -std=c99 -g

export CCNL_FLAGS
export CFLAGS

LIBS = ccnl-addons ccnl-core ccnl-fwd ccnl-pkt ccnl-nfn ccnl-unix
BINS = ccnl-relay

all: $(LIBS) $(BINS)

.PHONY: $(LIBS) $(BINS)

$(LIBS):
	$(MAKE) -C $@

$(BINS):
	$(MAKE) -C $@

clean:
	for LIB in $(LIBS); do \
		$(MAKE) -C $$LIB clean; \
	done
	for BIN in $(BINS); do \
    	$(MAKE) -C $$BIN clean; \
    done
