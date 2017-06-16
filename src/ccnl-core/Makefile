CC ?= gcc

CCNLCFLAGS = -Wall -Werror -std=c99 -g ${INCLUDES} -DCCNL_UNIX

ARFLAGS = -rcs

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

LIB = lib-ccnl-suite.a

$(OBJECTS): $(SOURCES)
	 $(CC) $(CCNLCFLAGS) -c $(SOURCES)


all: ${LIB}

${LIB}: ${OBJECTS}
	${AR} ${ARFLAGS} $@ $^
