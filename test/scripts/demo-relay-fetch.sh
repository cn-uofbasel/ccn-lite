#!/bin/sh

# demo-relay-fetch.sh -- test/demo for ccn-lite: CCNx relaying
USAGE="usage: sh demo-relay-fetch.sh SUITE CHANNEL KERNELMODULE\nwhere\n  SUITE= ccnb, ccnx2014, cisco2015, iot2014, ndn2013\n CHANNEL= udp, ux\n KERNELMODULE= true, false"
SET_CCNL_HOME_VAR="set system variable CCNL_HOME to your local CCN-Lite installation (.../ccn-lite) and run 'make clean all' in the src/ directory"
COMPILE_CCNL="run 'make clean all' in CCNL_HOME/src"

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

if [ ! -f "$CCNL_HOME/src/ccn-lite-relay" ]
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
elif [ $SUITE = "cisco2015" ] 
then
    DIR="cistlv"
    FWD="cisco"
    FNAME="chunked"
elif [ $SUITE = "iot2014" ] 
then
    DIR="iottlv"
    FWD="iot"
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
    if [ "$USEKRNL" = true ]
    then
        SOCKETA="u=$PORTA"
    else
        SOCKETA="-u$PORTA"
    fi
    SOCKETB="-u$PORTB"
    FACETOB="newUDPface any 127.0.0.1 $PORTB"
    PEEKADDR="-u 127.0.0.1/$PORTA"
elif [ "$CON" = "ux" ]
then
    SOCKETA=
    SOCKETB=
    FACETOB="newUNIXface $UXB"
    PEEKADDR=
else
    exit_error_msg "'$CON' is not a valid CON"
fi


# producing content 
# ----------------------------------------------------------------------

echo "chunkedtestcontent" | $CCNL_HOME/src/util/ccn-lite-produce -s $SUITE -c 5 -o $CCNL_HOME/test/$DIR "$FWD/$FNAME"

# ----------------------------------------------------------------------

# topology:

#  ccn-lite-peek --> relay A     -->  relay B
#
#                 127.0.0.1/9998   127.0.0.1/9999  (udp addresses)
#                 /tmp/a.sock      /tmp/b.sock (unix sockets, for ctrl only)

# ----------------------------------------------------------------------




echo -n "killing all ccn-lite-relay instances... "
killall ccn-lite-relay

# starting relay A, with a link to relay B

if [ "$USEKRNL" = true ]
then
    rmmod ccn_lite_lnxkernel
    insmod $CCNL_HOME/src/ccn-lite-lnxkernel.ko v=99 s=$SUITE $SOCKETA x=$UXA 
else
    $CCNL_HOME/src/ccn-lite-relay -v 100 -s $SUITE $SOCKETA -x $UXA 2>/tmp/a.log &
fi
sleep 1
FACEID=`$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXA $FACETOB | $CCNL_HOME/src/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
echo faceid=$FACEID
$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXA prefixreg $FWD $FACEID $SUITE | $CCNL_HOME/src/util/ccn-lite-ccnb2xml | grep ACTION

# starting relay B, with content loading
$CCNL_HOME/src/ccn-lite-relay -v 100 -s $SUITE $SOCKETB -x $UXB -d "$CCNL_HOME/test/$DIR" 2>/tmp/b.log &
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/src/util/ccn-lite-fetch -v trace -s$SUITE $PEEKADDR "$FWD/$FNAME" 2>/tmp/c.log >/tmp/res

RESULT=$?

# shutdown both relays
echo ""
echo "# Config of relay A:"
$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXA debug dump | $CCNL_HOME/src/util/ccn-lite-ccnb2xml
echo ""
echo "# Config of relay B:"
$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXB debug dump | $CCNL_HOME/src/util/ccn-lite-ccnb2xml

$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXA debug halt > /dev/null &
$CCNL_HOME/src/util/ccn-lite-ctrl -x $UXB debug halt > /dev/null &

sleep 1
killall ccn-lite-relay > /dev/null

if [ $RESULT = '0' ] 
then
    echo "=== FETCHED DATA ==="
    cat /tmp/res
    echo "\n=== FETCHED DATA ==="
else
    echo "ERROR exitcode $RESULT WHEN FETCHING DATA"
fi

if [ "$USEKRNL" = true ]
then
    rmmod ccn_lite_lnxkernel
fi

exit $RESULT

# eof
