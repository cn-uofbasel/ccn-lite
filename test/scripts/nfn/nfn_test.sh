#!/bin/bash

PACKETTYPELIST='ccnb ndn2013 ccnx2015 iot2014' #cisco2015   
#PACKETTYPELIST='ndn2013'

for PACKETTYPE in $PACKETTYPELIST; do

    #PACKETTYPE=ccnb

    echo 'Testing Packet Format' $PACKETTYPE

    echo '** killing all CCNL relay and Java instances'

    killall ccn-lite-relay 2> /dev/null
    killall ccn-nfn-relay 2> /dev/null
    killall java 2> /dev/null

    echo '** starting two CCNL relays'
    $CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9000 -x /tmp/mgmt1.sock 2> '/tmp/$PACKETTYPE-relay0.log' &
    $CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9001 -x /tmp/mgmt2.sock 2> '/tmp/$PACKETTYPE-relay1.log' &

    echo '** in 3 seconds, adding an UDP interface, twice'
    sleep 3

    $CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | $CCNL_HOME/bin/ccn-lite-ccnb2xml > '/tmp/$PACKETTYPE-command0.log'

    echo '** in 3 seconds, registering a prefix, twice'
    sleep 3

    $CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /nfn/node2 2 $PACKETTYPE | $CCNL_HOME/bin/ccn-lite-ccnb2xml > '/tmp/$PACKETTYPE-command1.log'

    echo '** in 3 seconds, starting the NFN compute server'
    sleep 3
    java -jar nfn.jar -s $PACKETTYPE -m /tmp/mgmt2.sock -o 9001 -p 9002 -d -r /nfn/node2 2>&1 >  '/tmp/$PACKETTYPE-computserver.log' &

    echo '** in 8 seconds, probing the NFN system'
    sleep 8

    RES=`ccn-lite-simplenfn -v trace -s $PACKETTYPE -u '127.0.0.1/9000' 'call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md' | ccn-lite-pktdump -f 2`
    RC=$?
    echo 'Return code: $RC, result: '$RES''

    if [ 0 -ne $RC ] || [ 3744 -ne $RES ]; then
         echo '\e[0;91m[FAILED] Something went wrong\e[0;0m'
    else
         echo '\e[0;92m [OK] Test was successful \e[0;0m'
    fi

    $CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock debug halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /dev/null
    $CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock debug halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /dev/null

    killall ccn-lite-relay 2> /dev/null
    killall ccn-nfn-relay  2> /dev/null
    killall java  2> /dev/null

done
