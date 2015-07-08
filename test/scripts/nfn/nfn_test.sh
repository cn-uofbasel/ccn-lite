#!/bin/bash

if [ ! -f nfn.jar ]; then
    printf 'No NFN executable available, downloading [..]'
    wget https://github.com/cn-uofbasel/nfn-scala/releases/download/v0.1.0/nfn.jar &> /dev/null
    echo $'\b\b\b\e[1;32mdone\e[0;0m]'
fi

EXITCODE=0
PACKETTYPELIST='ccnb ndn2013 iot2014' #ccnx2015 cisco2015
#PACKETTYPELIST='ndn2013'
DEBUGOUT="FALSE"
echo "Testing NFN with a sample computation. Two NFN nodes, one is connected to a NFN scala computation server:"

for PACKETTYPE in $PACKETTYPELIST; do

    printf '    Testing Packet Format %-10s [..]' $PACKETTYPE

    #echo '** killing all CCNL relay and Java instances'

    killall ccn-lite-relay 2> /dev/null
    killall ccn-nfn-relay 2> /dev/null
    killall java 2> /dev/null

    sleep 2


    printf '\b\b\bStarting two CCNL relays..]'
    #echo '** starting two CCNL relays'
    "$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9000 -x /tmp/mgmt1.sock 2> "/tmp/$PACKETTYPE-relay0.log" &
    "$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9001 -x /tmp/mgmt2.sock 2> "/tmp/$PACKETTYPE-relay1.log" &


    #echo '** in 3 seconds, adding an UDP interface, twice'
    sleep 3

    printf '\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bAdding an UDP interface..]  \b\b'
    "$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" > "/tmp/$PACKETTYPE-command0.log"

    #echo '** in 3 seconds, registering a prefix, twice'
    sleep 3

    printf '\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bRegistering a prefix..]    \b\b\b\b'
    "$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock prefixreg /nfn/node2 2 "$PACKETTYPE" | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" > "/tmp/$PACKETTYPE-command1.log"

    #echo '** in 3 seconds, starting the NFN compute server'
    sleep 3


    printf '\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bStarting the NFN compute server..]    \b\b\b\b'
    java -jar "$CCNL_HOME/test/scripts/nfn/nfn.jar" -s "$PACKETTYPE" -m /tmp/mgmt2.sock -o 9001 -p 9002 -d -r /nfn/node2 2>&1 >  "/tmp/$PACKETTYPE-computserver.log" &

    #echo '** in 8 seconds, probing the NFN system'
    sleep 8

    RES=`$CCNL_HOME/bin/ccn-lite-simplenfn -s "$PACKETTYPE" -u '127.0.0.1/9000' 'call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md' 2> /dev/null | $CCNL_HOME/bin/ccn-lite-pktdump -f 2`
    RC=$?
    #echo "Return code: $RC, result: '$RES'"

    if [ 0 -ne "$RC" ] || [ 3558 -ne "$RES" ]; then
         echo $'\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\e[1;31mFAILED\e[0;0m]                                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b'
         EXITCODE=1
    else
         echo $'\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\e[1;32mOK\e[0;0m]                                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b'
    fi

    "$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" > /dev/null
    "$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt2.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" > /dev/null

    killall ccn-lite-relay 2> /dev/null
    killall ccn-nfn-relay  2> /dev/null
    killall java  2> /dev/null

done
exit $EXITCODE
