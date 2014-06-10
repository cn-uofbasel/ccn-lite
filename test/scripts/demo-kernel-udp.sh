#!/bin/sh

# demo-kernel-udp.sh -- test/demo for ccn-lite:
# CCNx relaying via UDP sockets, where the CCN relay sits in the Linux kernel

# execute with sudo!

. ./paths.sh

PORTA=9998
PORTB=9999
UXA=/tmp/ccn-lite-lnxmodule-$$.sock

# topology:

#  ccn-lite-peek --> kernel-relay A   -->  relay B
#
#                   127.0.0.1/9998    127.0.0.1/9999  (udp addresses)
#                   /tmp/a.sock (only ctrl)

# ----------------------------------------------------------------------

echo -n "** Killing all ccn-lite relay instances... "
killall ccn-lite-relay 2>/dev/null
rmmod ccn-lite-lnxkernel 2>/dev/null
echo

echo "** Configuring kernel module"
# starting (kernel) relay A, with a link to relay B
insmod $CCNL_HOME/ccn-lite-lnxkernel.ko v=99 u=$PORTA x=$UXA
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $UXA newUDPface any 127.0.0.1 $PORTB | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg /ccnx/0.7.1/doc/technical $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

echo "** Starting user space relay"
# starting relay B, with content loading
$CCNL_HOME/ccn-lite-relay -v 99 -u $PORTB -d $CCNL_HOME/test/ccnb 2>/tmp/b.log &
sleep 1

echo "** Starting test"
# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-peek -u 127.0.0.1/$PORTA /ccnx/0.7.1/doc/technical/NameEnumerationProtocol.txt | $CCNL_HOME/util/ccn-lite-pktdump

# shutdown both relays
rmmod ccn-lite-lnxkernel
killall ccn-lite-relay

# eof
