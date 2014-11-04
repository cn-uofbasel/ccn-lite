# ccnl^/test/unit/Makefile

CC?=gcc
MYCFLAGS= -Wall -g -O0 
EXTLIBS=  -lcrypto

PROGS= $(wildcard *.c) 


all: unit

unit: unit.c
	$(CC) $(MYCFLAGS) -o $@ $<  $(EXTLIBS)


clean:
	rm -f unit
