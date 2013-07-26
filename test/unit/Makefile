# ccnl/test/unit/Makefile

CC?=gcc
MYCFLAGS= -Wall -g -O0 -I../..
EXTLIBS=  -lcrypto

PROGS=	prefix-cmp

all: $(PROGS)

prefix-cmp: Makefile prefix-cmp.c ../../ccnx.h ../../ccnl.h ../../ccnl-core.h ../../ccnl-core.c
	$(CC) $(MYCFLAGS) prefix-cmp.c -o $@ $(EXTLIBS)

clean:
	rm -rf *~ $(PROGS)
