CC ?= gcc

SUITEFLAGS = -DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB \
			 -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC
USEFLAGS = -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING
CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} ${USEFLAGS} ${SUITEFLAGS}

ARFLAGS = -rcs

#SOURCES=$(wildcard *.c)
SOURCES = ccnl-pkt-switch.c ccnl-pkt-ndntlv.c ccnl-pkt-ccntlv.c \
		  ccnl-pkt-ccnb.c ccnl-pkt-cistlv.c ccnl-pkt-iottlv.c ccnl-pkt-localrpc.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

LIB = libccnl-pkt.a

all: ${LIB}

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^ ../ccnl-core/libccnl-core.a ../ccnl-addons/libccnl-addons.a

clean: 
	rm *.o *.a
