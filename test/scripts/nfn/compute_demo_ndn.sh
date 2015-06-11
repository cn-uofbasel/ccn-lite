#!/bin/bash


PACKETTYPE=ndn2013

echo "** killing all CCNL relay and Python instances"

killall ccn-lite-relay 2> /dev/null
killall ccn-nfn-relay 2> /dev/null
killall java 2> /dev/null

echo "** starting two CCNL relays"
$CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9000 -x /tmp/mgmt1.sock 2>/tmp/relay0.log &
$CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9001 -x /tmp/mgmt2.sock 2>/tmp/relay1.log &

echo "** in 3 seconds, adding an UDP interface, twice"
sleep 3

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /tmp/command1.log

echo "** in 3 seconds, registering a prefix, twice"
sleep 3

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /nfn/node2 2 $PACKETTYPE | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /tmp/command2.log

echo "** in 3 seconds, starting the dummy compute server"
sleep 3
java -jar nfn.jar -m /tmp/mgmt2.sock -o 9001 -p 9002 -d -r /nfn/node2 2>&1 >  /tmp/computserver.log &  

echo "** in 8 seconds, probing the NFN system"
sleep 8

RES=`ccn-lite-simplenfn  -u 127.0.0.1/9000 "call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md" | ccn-lite-pktdump -f 2`
echo "Result: " $RES


if [ $RES = 3756 ]
then
     echo -e "\e[0;92m [OK] Test was successful \e[0;0m"
else 
     echo -e "\e[0;91m[FAILED] Sth. went wrong\e[0;0m"
fi

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock debug halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /dev/null
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock debug halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml > /dev/null

killall ccn-lite-relay 2> /dev/null
killall ccn-nfn-relay  2> /dev/null
killall java  2> /dev/null
