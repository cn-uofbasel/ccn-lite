#!/bin/sh

# demo-relay-fetch.sh -- test/demo for ccn-lite: starts single relay to show fetch with produced content

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

echo -n "killing all ccn-lite-relay instances... "
killall ccn-lite-relay

echo
# starting relay A, with a link to relay B
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTA -s 2 -x $UXA 2>/tmp/a.log &
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $UXA newUDPface any 127.0.0.1 $PORTB | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg /ndn $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION

# starting relay B, with content loading
$CCNL_HOME/ccn-lite-relay -s 2 -v 99 -u $PORTB -x $UXB -d $CCNL_HOME/test/ndntlv 2>/tmp/b.log &
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-peek -u "127.0.0.1/$PORTA" -s 2 "/ndn/abc" 




# shutdown both relays
echo ""
echo "# Config of relay A:"
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml
echo ""
echo "# Config of relay B:"
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug halt > /dev/null &
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug halt > /dev/null &

sleep 1
killall ccn-lite-ctrl > /dev/null

# eof
