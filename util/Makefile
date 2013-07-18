# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -g

PROGS=	ccn-lite-ctrl \
	ccn-lite-parse \
	ccn-lite-peek \
	ccn-lite-put

all: $(PROGS)

ccn-lite-ctrl: ccn-lite-ctrl.c ../ccnx.h ../ccnl.h

ccn-lite-parse: ccn-lite-parse.c ../ccnx.h ../ccnl.h

ccn-lite-peek: ccn-lite-peek.c ../ccnx.h ../ccnl.h

ccn-lite-put: ccn-lite-put.c ../ccnx.h ../ccnl.h

demo: $(PROGS)
	./ccnl-mkInterest ccn-lite says hello world äöü | ./ccn-lite-parse

clean:
	rm -rf *~ $(PROGS)
