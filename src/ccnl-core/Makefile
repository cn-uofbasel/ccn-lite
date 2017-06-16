CC ?= gcc

CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} -DCCNL_UNIX -DUSE_IPV4

ARFLAGS = -rcs

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

LIB = libccnl-core.a

all: ${LIB}

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^ ../ccnl-addons/libccnl-addons.a


clean: 
	rm *.o *.a
