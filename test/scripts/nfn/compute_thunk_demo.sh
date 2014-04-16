killall ccn-lite-relay
killall python

$CCNL_HOME/ccn-lite-relay -v 99 -u 9000 -x /tmp/mgmt1.sock 2> /tmp/r0.log &
$CCNL_HOME/ccn-lite-relay -v 99 -u 9001 -x /tmp/mgmt2.sock 2> /tmp/r1.log &

sleep 3

$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | $CCNL_HOME/util/ccn-lite-ccnb2xml
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt2.sock newUDPface any 127.0.0.1 9002  | $CCNL_HOME/util/ccn-lite-ccnb2xml

sleep 3

$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt1.sock prefixreg /test 2  | $CCNL_HOME/util/ccn-lite-ccnb2xml
$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt2.sock prefixreg /COMPUTE 2  | $CCNL_HOME/util/ccn-lite-ccnb2xml

sleep 3

$CCNL_HOME/util/ccn-lite-ctrl -x /tmp/mgmt2.sock addContentToCache $CCNL_HOME/test/ccnb/nfn/computation_content.ccnb | $CCNL_HOME/util/ccn-lite-ccnb2xml

sleep 3

python $CCNL_HOME/test/scripts/nfn/dummyanswer.py & > /dev/null

sleep 3

$CCNL_HOME/util/ccn-lite-peekcomputation -t -u 127.0.0.1/9000 -w 100 "(@x add 6 (call 1 x))" "/test/data" | $CCNL_HOME/util/ccn-lite-ccnb2hex

sleep 5

$CCNL_HOME/util/ccn-lite-peekcomputation -u 127.0.0.1/9000 -w 100 "(@x add 6 (call 1 x))" "/test/data" | $CCNL_HOME/util/ccn-lite-ccnb2hex
