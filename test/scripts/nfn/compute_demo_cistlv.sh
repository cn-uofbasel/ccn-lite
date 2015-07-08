#!/bin/bash
# compute_demo_cistlv.sh

killall ccn-lite-relay
killall ccn-nfn-relay
killall python

echo "** starting two CCNL relays"
$CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9000 -x /tmp/mgmt1.sock 2>/tmp/r0.log &
$CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9001 -x /tmp/mgmt2.sock 2>/tmp/r1.log &

echo "** in 3 seconds, adding an UDP interface, twice"
sleep 3

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | $CCNL_HOME/bin/ccn-lite-ccnb2xml
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock newUDPface any 127.0.0.1 9002  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "** in 3 seconds, registering a prefix, twice"
sleep 3

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /test 2 cisco2015| $CCNL_HOME/bin/ccn-lite-ccnb2xml
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock prefixreg /COMPUTE 2 cisco2015 | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "** in 3 seconds, adding content"
sleep 3

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock addContentToCache $CCNL_HOME/test/cistlv/nfn/computation_content.cistlv | $CCNL_HOME/src/util/ccn-lite-ccnb2xml

echo "** in 3 seconds, starting the dummy compute server"
sleep 3

python $CCNL_HOME/test/scripts/nfn/dummyanswer_cistlv.py & > /dev/null

echo "** in 3 seconds, retrieving a computation result"
sleep 3

$CCNL_HOME/bin/ccn-lite-peek -v trace -s cisco2015 -u 127.0.0.1/9000 -w 100 "" "call 1 /test/data" 2>/tmp/r-c.log | $CCNL_HOME/bin/ccn-lite-pktdump

$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt1.sock debug dump+halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml >/tmp/r0-end.log
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt2.sock debug dump+halt | $CCNL_HOME/bin/ccn-lite-ccnb2xml >/tmp/r1-end.log

killall ccn-lite-relay 2>/dev/null
killall ccn-nfn-relay 2>/dev/null
killall python 2>/dev/null
