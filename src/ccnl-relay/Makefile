CC ?= gcc

SUITEFLAGS = -DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB \
			 -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC
USEFLAGS = -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING
CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} ${USEFLAGS} ${SUITEFLAGS}

LIBS = ../ccnl-addons/lib/libccnl-addons.a \
	   ../ccnl-pkt/lib/libccnl-pkt.a \
	   ../ccnl-fwd/lib/libccnl-fwd.a \
	   ../ccnl-unix/lib/libccnl-unix.a \
	   ../ccnl-core/lib/libccnl-core.a

IFLAGS = -I../ccnl-addons/include \
		 -I../ccnl-pkt/include \
		 -I../ccnl-fwd/include \
		 -I../ccnl-unix/include \
		 -I../ccnl-core/include

#ARFLAGS = -rcs

#SOURCES=$(wildcard *.c)
#SOURCES = ccn-lite-relay.c
#OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: ccn-lite-relay

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

ccn-lite-relay: ccn-lite-relay.c
	$(CC) -o $@ $(CCNLCFLAGS) -DUSE_LOGGING $< $(EXTLIBS) $(IFLAGS) $(LIBS)
	@$(CREATE_BIN)

clean: 
	rm -f ccn-lite-relay

#libccnl-core.a ../ccnl-addons/libccnl-addons.a libccnl-pkt.a libccnl-fwd.a ../ccnl-unix/libccnl-unix.a
