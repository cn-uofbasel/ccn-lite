#!/bin/bash
# ccn-lite/test/scripts/interop-peering-LDDL.sh

cat <<EOF >/dev/null

This is a script to start a local interop test where two CCN-lite and
two ccnd relays are peered via UDP in a string topology.

The following diagram shows this "LDDL" configuration and also
documents the chosen UDP ports and UNIX IPC socket names:


                                                ccn-lite-peek
                                                      |
    ccn-lite-relay <----> ccnd <----> ccnd <--> ccn-lite-relay
    (w/content)

UDP:  9001                9002        9003           9004
UX:  /tmp/.1.sock   /tmp/.2.sock  /tmp/.3.sock    /tmp/.4.sock

created 2013-04-10 <christian.tschudin@unibas.ch>

EOF


CCND_HOME=~/ccn-lite/ccnx-0.7.1
CCND_PORTA=9002
CCND_PORTB=9003
CCND_UXA=/tmp/.2.sock
CCND_UXB=/tmp/.3.sock
CCND_LOGA=/tmp/LDDL-2.log
CCND_LOGB=/tmp/LDDL-3.log

CCNL_HOME=~/ccn-lite/ccn-lite
CCNL_PORTA=9001
CCNL_PORTB=9004
CCNL_UXA=/tmp/.1.sock
CCNL_UXB=/tmp/.4.sock
CCNL_LOGA=/tmp/LDDL-1.log
CCNL_LOGB=/tmp/LDDL-4.log


# cleanup the machine:
echo "** Cleaning the machine from old log files and sockets ..."

killall ccn-lite-relay
export CCN_LOCAL_SOCKNAME=$CCND_UXA
$CCND_HOME/bin/ccndstop
export CCN_LOCAL_SOCKNAME=$CCND_UXB
$CCND_HOME/bin/ccndstop
killall ccnd
rm -rf /tmp/.[14].sock
rm -rf /tmp/.[23].sock.*


# start the four relays, create the FWD entries:
echo "** Starting up the four relays - this takes 5 seconds or so ..."

$CCNL_HOME/ccn-lite-relay -u $CCNL_PORTA -x $CCNL_UXA \
		-v 99 -d $CCNL_HOME/test/ccnb >$CCNL_LOGA 2>&1 &

export CCN_LOCAL_PORT=$CCND_PORTA
export CCN_LOCAL_SOCKNAME=$CCND_UXA
export CCND_LOG=$CCND_LOGA
export CCND_DEBUG=31
$CCND_HOME/bin/ccndstart
sleep 1
$CCND_HOME/bin/ccndc add /ccnx/0.7.1/doc udp 127.0.0.1 $CCNL_PORTA

export CCN_LOCAL_PORT=$CCND_PORTB
export CCN_LOCAL_SOCKNAME=$CCND_UXB
export CCND_LOG=$CCND_LOGB
$CCND_HOME/bin/ccndstart
sleep 1
$CCND_HOME/bin/ccndc add /ccnx/0.7.1/doc udp 127.0.0.1 $CCND_PORTA

$CCNL_HOME/ccn-lite-relay -u $CCNL_PORTB -x $CCNL_UXB \
		-v 99 >$CCNL_LOGB 2>&1 &
sleep 1
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB newUDPface any 127.0.0.1 $CCND_PORTB
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB prefixreg /ccnx/0.7.1/doc 2
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB debug dump


echo "** Proceeding with the transfer test in 2 seconds, please stand by ..."
sleep 2
echo "** Received content:"


# test access to content:

$CCNL_HOME/util/ccn-lite-peek -u 127.0.0.1/$CCNL_PORTB /ccnx/0.7.1/doc/technical/URI.txt | $CCNL_HOME/util/ccn-lite-parse
echo

# shut down all relays:
echo "** Shutting down all relays ..."

$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXA debug dump+halt

export CCN_LOCAL_SOCKNAME=$CCND_UXA
$CCND_HOME/bin/ccndstop

export CCN_LOCAL_SOCKNAME=$CCND_UXB
$CCND_HOME/bin/ccndstop

$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UXB debug dump+halt


#

echo "**"
echo "** END_OF_TEST"

echo
echo "** how the 1st node saw things:"
egrep '(interest for|content_new).*/doc/' $CCNL_LOGB
echo
echo "** how the 2nd node saw things:"
egrep '(interest_from|content_from).*/doc/' $CCND_LOGB
echo
echo "** how the 3rd node saw things:"
egrep '(interest_from|content_from).*/doc/' $CCND_LOGA
echo
echo "** how the 4th node saw things (content was injected at start):"
egrep '(interest for|content_new).*/doc.*URI' $CCNL_LOGA

echo
echo "** Find more in the logs:"
echo "** $CCNL_LOGA $CCND_LOGA $CCND_LOGB $CCNL_LOGB"

# eof
