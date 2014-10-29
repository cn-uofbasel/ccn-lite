#!/bin/sh

# demo-relay-udp.sh -- test/demo for ccn-lite: CCNx relaying via UDP sockets
USAGE="usage: sh demo-relay.sh <SUITE[ccnb,ccnx2014,ndn2013]> <CON[udp,ux]> <USEKRNL[true,false]"
SET_CCNL_HOME_VAR="set system variable CCNL_HOME to your local CCN-Lite installation (.../ccn-lite) and run 'make clean all'"
COMPILE_CCNL="run 'make clean all' in CCNL_HOME"

exit_error_msg () {
    echo $1
    echo $USAGE
    exit 1
}

if [ -z "$CCNL_HOME" ]
then
    echo $SET_CCNL_HOME_VAR
    exit 1
fi

if [ ! -f "$CCNL_HOME/ccn-lite-relay" ]
then
    echo $COMPILE_CCNL
    exit 1
fi

if [ "$#" -ne 3 ]; then
    exit_error_msg "illegal number of parameters"
fi

PORTA=9998
PORTB=9999
UXA=/tmp/ccn-lite-relay-$$-a.sock
UXB=/tmp/ccn-lite-relay-$$-b.sock

SUITE=$1
CON=$2
USEKRNL=$3

# suite setup
if [ $SUITE = "ccnb" ] 
then
    DIR="ccnb"
    FWD="/ccnx/0.7.1/doc/technical"
    FNAME="NameEnumerationProtocol.txt"
elif [ $SUITE = "ccnx2014" ] 
then
    DIR="ccntlv"
    FWD="ccnx"
    FNAME="chunked"
elif [ $SUITE = "ndn2013" ] 
then
    DIR="ndntlv"
    FWD="ndn"
    FNAME="chunked"
else
    exit_error_msg "'$SUITE' is not a valid SUITE"
fi

# face setup
if [ "$CON" = "udp" ]
then
    if [ '$USEKRNL' = true ]
    then
        SOCKETA="u=$PORTA"
    else
        SOCKETA="-u$PORTA"
    fi
    SOCKETB="-u$PORTB"
    FACETOB="newUDPface any 127.0.0.1 $PORTB"
    PEEKADDR="-u 127.0.0.1/$PORTB"
elif [ "$CON" = "ux" ]
then
    SOCKETA=
    SOCKETB=
    FACETOB="newUNIXface $UXB"
    PEEKADDR=
else
    exit_error_msg "'$CON' is not a valid CON"
fi


# topology:

#  ccn-lite-peek --> relay A     -->  relay B
#
#                 127.0.0.1/9998   127.0.0.1/9999  (udp addresses)
#                 /tmp/a.sock      /tmp/b.sock (unix sockets, for ctrl only)

# ----------------------------------------------------------------------

echo -n "killing all ccn-lite-relay instances... "
killall ccn-lite-relay

if [ '$USEKRNL' = true ]
then
    rmmod ccn-lite-lnxkernel 2>/dev/null
fi

echo
# starting relay A, with a link to relay B

if [ '$USEKRNL' = true ]
then
    insmod $CCNL_HOME/ccn-lite-lnxkernel.ko v=99 $SOCKETA x=$UXA
else
    $CCNL_HOME/ccn-lite-relay -v 99 -s $SUITE $SOCKETA -x $UXA 2>/tmp/a.log &
fi
sleep 1
FACEID=`$CCNL_HOME/util/ccn-lite-ctrl -x $UXA $FACETOB | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
echo $FACEID
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg $FWD $FACEID $SUITE | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION

# starting relay B, with content loading
$CCNL_HOME/ccn-lite-relay -v 99 -s $SUITE $SOCKETB -x $UXB -d "$CCNL_HOME/test/$DIR" 2>/tmp/b.log &
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-fetch -s$SUITE $PEEKADDR "$FWD/$FNAME" > /tmp/res

RESULT=$?

# shutdown both relays
# echo ""
# echo "# Config of relay A:"
# $CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml
# echo ""
# echo "# Config of relay B:"
# $CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump | $CCNL_HOME/util/ccn-lite-ccnb2xml

$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug halt > /dev/null &
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug halt > /dev/null &

sleep 1
killall ccn-lite-ctrl > /dev/null

if [ $RESULT = '0' ] 
then
    echo "=== FETCHED DATA ==="
    cat /tmp/res
    echo "\n=== FETCHED DATA ==="
else
    echo "ERROR exitcode $RESULT WHEN FETCHING DATA"
fi

exit $RESULT

# eof
