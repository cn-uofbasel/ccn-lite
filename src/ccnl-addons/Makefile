CC ?= gcc

CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} -DCCNL_UNIX

#ARFLAGS = rcs
ARFLAGS = -static -o
AR = libtool

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

LIB = libccnl-addons.a

all: ${LIB}

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^


clean: 
	rm *.o *.a
