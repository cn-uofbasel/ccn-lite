#!/bin/sh
# ccn-lite/test/scripts/interop-peering-XLLX.sh

cat <<EOF >/dev/null

This is a script to start a local interop test
where two CCN-lite (L) and two ccnd relays (X)
are peered via UDP in a string topology.

The following diagram shows this "XLLX" configuration
and also documents the chosen UDP ports and UNIX IPC
socket names:


                                                           ccnpeek
                                                              |
       ccnd <----> ccn-lite-relay <--> ccn-lite-relay <----> ccnd 
    (w/content)

UDP:  9001                9002             9003              9004
UX:  /tmp/.1.sock   /tmp/.2.sock       /tmp/.3.sock      /tmp/.4.sock

created 2013-04-10 <christian.tschudin@unibas.ch>

EOF

. ./paths.sh

CCND_PORTA=9001
CCND_PORTB=9004
CCND_UXA=/tmp/.1.sock
CCND_UXB=/tmp/.4.sock
CCND_LOGA=/tmp/XLLX-1.log
CCND_LOGB=/tmp/XLLX-4.log

CCNL_PORTA=9002
CCNL_PORTB=9003
CCNL_UXA=/tmp/.2.sock
CCNL_UXB=/tmp/.3.sock
CCNL_LOGA=/tmp/XLLX-2.log
CCNL_LOGB=/tmp/XLLX-3.log


# cleanup the machine:
echo -n "** Cleaning the machine from old log files and sockets ..."

killall ccn-lite-relay
export CCN_LOCAL_SOCKNAME=$CCND_UXA
$CCND_HOME/bin/ccndstop
export CCN_LOCAL_SOCKNAME=$CCND_UXB
$CCND_HOME/bin/ccndstop
killall ccnd
rm -rf /tmp/.[23].sock
rm -rf /tmp/.[14].sock.*


# start the four relays, create the FWD entries:
echo
echo "** Starting up the four relays - this takes 5 seconds or so ..."

export CCN_LOCAL_PORT=$CCND_PORTA
export CCN_LOCAL_SOCKNAME=$CCND_UXA
export CCND_LOG=$CCND_LOGA
#export CCND_DEBUG=31
export CCND_DEBUG=1023
$CCND_HOME/bin/ccndstart
sleep 1
$CCND_HOME/bin/ccndsmoketest send $CCNL_HOME/test/ccnb/URI.txt.ccnb

$CCNL_HOME/ccn-lite-relay -u $CCNL_PORTA -x $CCNL_UXA -v 99 >$CCNL_LOGA 2>&1 &
sleep 1

FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXA newUDPface any 127.0.0.1 $CCND_PORTA | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXA prefixreg /ccnx/0.7.1/doc $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION

echo
echo "# Configuration of node A:"
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml


$CCNL_HOME/ccn-lite-relay -u $CCNL_PORTB -x $CCNL_UXB -v 99 >$CCNL_LOGB 2>&1 &
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB newUDPface any 127.0.0.1 $CCNL_PORTA | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB prefixreg /ccnx/0.7.1/doc $FACEID | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION

echo
echo "# Configuration of node A:"
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

export CCN_LOCAL_PORT=$CCND_PORTB
export CCN_LOCAL_SOCKNAME=$CCND_UXB
export CCND_LOG=$CCND_LOGB
$CCND_HOME/bin/ccndstart
sleep 1
$CCND_HOME/bin/ccndc add /ccnx/0.7.1/doc udp 127.0.0.1 $CCNL_PORTB



echo "** Proceeding with the transfer test in 2 seconds, please stand by ..."
sleep 2
echo "** Received content:"


# test access to content:

$CCND_HOME/bin/ccnpeek /ccnx/0.7.1/doc/technical/URI.txt | $CCNL_HOME/util/ccn-lite-pktdump
echo

# shut down all relays:
echo "** Shutting down all relays ..."

export CCN_LOCAL_SOCKNAME=$CCND_UXA
$CCND_HOME/bin/ccndstop

echo
echo "# Configuration of node A:"
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXA debug dump+halt | $CCNL_HOME/util/ccn-lite-ccnb2xml
echo
echo "# Configuration of node B:"
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB debug dump+halt | $CCNL_HOME/util/ccn-lite-ccnb2xml

export CCN_LOCAL_SOCKNAME=$CCND_UXB
$CCND_HOME/bin/ccndstop


#

echo "**"
echo "** END_OF_TEST"

echo
echo "** how the 1st node saw things:"
egrep '(interest_from|content_from).*/doc/' $CCND_LOGB
echo
echo "** how the 2nd node saw things:"
egrep '(interest=|content=).*/doc/' $CCNL_LOGB
echo
echo "** how the 3rd node saw things:"
egrep '(interest=|content=).*/doc/' $CCNL_LOGA
echo
echo "** how the 4th node saw things (content was injected at start):"
egrep '(interest_from|content_from|content_to).*/doc/' $CCND_LOGA

echo
echo "** Find more in the logs:"
echo "** $CCND_LOGA $CCNL_LOGA $CCNL_LOGB $CCND_LOGB"

# eof
