CC ?= gcc

CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} -DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PREFIX_MATCHING

#ARFLAGS = rcs
ARFLAGS = -static -o
AR = libtool

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

LIB = libccnl-core.a

all: ${LIB}

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^

clean: 
	rm *.o *.a
