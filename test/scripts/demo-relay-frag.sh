#!/bin/sh

# demo-relay-frag.sh -- test/demo for ccn-lite:
# CCNx relaying via UDP sockets, using fragments of length 600

. ./paths.sh

PORTA=9998
PORTB=9999
UXA=/tmp/ccn-lite-relay-$$-a.sock
UXB=/tmp/ccn-lite-relay-$$-b.sock

PROTO=ccnx2013
# PROTO=seqd2012

# topology:

#  ccn-lite-peek --> relay A     -->  relay B (with content)
#
#                 127.0.0.1/9998   127.0.0.1/9999  (udp addresses)
#                 /tmp/a.sock      /tmp/b.sock (unix sockets, for ctrl only)

# ----------------------------------------------------------------------

echo -n "killing all ccn-lite-relay instances... "
killall ccn-lite-relay
echo

# starting relay A, with a link to relay B
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTA -x $UXA 2>/tmp/a.log &
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $UXA newUDPface any 127.0.0.1 $PORTB | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg /ccnx/0.7.1/doc/technical $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA setfrag $FACEID $PROTO 600 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

# starting relay B, content loading
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTB -x $UXB -d $CCNL_HOME/test/ccnb 2>/tmp/b.log &
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $UXB newUDPface any 127.0.0.1 $PORTA | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB setfrag $FACEID $PROTO 600 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-peek -u 127.0.0.1/$PORTA /ccnx/0.7.1/doc/technical/NameEnumerationProtocol.txt | $CCNL_HOME/util/ccn-lite-pktdump

# shutdown both relays
echo "# Config of node A:"
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml
echo "# Config of node B:"
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug halt > /dev/null
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug halt > /dev/null

echo
echo Sender side:
egrep 'fragmenting' /tmp/b.log
echo
echo Receiver side:
egrep '(bytes from face.*0\.2)|(>>)' /tmp/a.log
# eof
