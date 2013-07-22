# ccnl/util/Makefile

CC?=gcc
CFLAGS=-Wall -g

PROGS=	ccn-lite-ctrl \
	ccn-lite-ccnb2hex \
	ccn-lite-ccnb2xml \
	ccn-lite-peek

all: $(PROGS)

ccn-lite-ctrl: ccn-lite-ctrl.c ../ccnx.h ../ccnl.h

ccn-lite-ccnb2hex: ccn-lite-ccnb2hex.c ../ccnx.h ../ccnl.h

ccn-lite-ccnb2xml: ccn-lite-ccnb2xml.c ../ccnx.h ../ccnl.h

ccn-lite-peek: ccn-lite-peek.c ../ccnx.h ../ccnl.h

demo: $(PROGS)
	./ccnl-mkInterest ccn-lite says hello world äöü | ./ccn-lite-ccnb2hex
	./ccnl-mkInterest ccn-lite says hello world äöü | ./ccn-lite-ccnb2xml

clean:
	rm -rf *~ $(PROGS)
