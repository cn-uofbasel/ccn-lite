#!/bin/bash

# relay-udp.sh -- test CCN relaying via UDP sockets

CCNL_HOME=../..

PORTA=9998
PORTB=9999
UXA=/tmp/ccn-lite-relay-$$-a.sock
UXB=/tmp/ccn-lite-relay-$$-b.sock

# topology:

#  ccn-lite-peek --> relay A     -->  relay B
#
#                 127.0.0.1/9998   127.0.0.1/9999  (udp addresses)
#                 /tmp/a.sock      /tmp/b.sock (unix sockets, for ctrl only)

# ----------------------------------------------------------------------

echo killing all ccn-lite-relay instances:
killall ccn-lite-relay

# starting relay A, with a link to relay B
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTA -x $UXA 2>/tmp/a.log &
sleep 1
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA newUDPface any 127.0.0.1 $PORTB
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg /ccnx/0.7.1/doc/technical 2

# starting relay B, with content loading
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTB -x $UXB -d $CCNL_HOME/test/ccnb 2>/tmp/b.log &
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-peek -u 127.0.0.1/$PORTA /ccnx/0.7.1/doc/technical/NameEnumerationProtocol.txt | $CCNL_HOME/util/ccn-lite-parse

# shutdown both relays
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump+halt
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump+halt

# eof
