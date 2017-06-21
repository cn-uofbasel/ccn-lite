CC ?= gcc

SUITEFLAGS = -DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB \
			 -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC
USEFLAGS = -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING
CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} ${USEFLAGS} ${SUITEFLAGS}

#ARFLAGS = -rcs

#SOURCES=$(wildcard *.c)
#SOURCES = ccn-lite-relay.c
#OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: ccn-lite-relay

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

ccn-lite-relay: ccn-lite-relay.c
	${CC} -o $@ ${CCNLCFLAGS} -DUSE_LOGGING $< ${EXTLIBS} \
		../ccnl-core/libccnl-core.a ../ccnl-addons/libccnl-addons.a ../ccnl-pkt/libccnl-pkt.a ../ccnl-fwd/libccnl-fwd.a ../ccnl-unix/libccnl-unix.a
	@${CREATE_BIN}

clean: 
	rm ccn-lite-relay
