#!/bin/sh

echo "Create face A->B"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock newUDPface any 127.0.0.1 9002 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule A->B for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule A->B for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml


echo "Create face B->C"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock newUDPface any 127.0.0.1 9003 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule B->C for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule A->B for /node/nodeA. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml


echo "Create face C->D"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock newUDPface any 127.0.0.1 9004 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule C->D for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule A->B for /node/nodeA. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml