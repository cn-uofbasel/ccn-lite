CC ?= gcc

SUITEFLAGS = -DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB \
			 -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC
USEFLAGS = -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING -DNEEDS_PREFIX_MATCHING
CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} ${USEFLAGS} ${SUITEFLAGS}

#ARFLAGS = rcs
ARFLAGS = -static -o
AR = libtool

#SOURCES=$(wildcard *.c)
SOURCES = ccnl-fwd.c ccnl-dispatch.c ccnl-localrpc.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

LIB = libccnl-fwd.a

all: ${LIB}

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^

clean: 
	rm *.o *.a
