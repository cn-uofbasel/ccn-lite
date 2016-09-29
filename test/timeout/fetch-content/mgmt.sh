#!/bin/sh

echo "Create face A->B"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock newUDPface any 127.0.0.1 9002 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule A->B for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule A->B for /node/nodeF. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /node/nodeF $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule A->B for /node/nodeD. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-a.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml



echo "Create face B->C"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock newUDPface any 127.0.0.1 9003 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule B->C for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule C->C for /node/nodeF. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock prefixreg /node/nodeF $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule B->C for /node/nodeD. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-b.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml



echo "Create face C->D"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock newUDPface any 127.0.0.1 9004 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule C->D for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule C->D for /node/nodeF. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock prefixreg /node/nodeF $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule C->D for /node/nodeD. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-c.sock prefixreg /node/nodeD $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml



echo "Create face D->E"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-d.sock newUDPface any 127.0.0.1 9005 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule D->E for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-d.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule D->E for /node/nodeF. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-d.sock prefixreg /node/nodeF $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml



echo "Create face E->F"
FACEID=`$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-e.sock newUDPface any 127.0.0.1 9006 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml | grep FACEID | sed -E 's/^[^0-9]*([0-9]+).*/\1/'`

echo "Create forwarding rule E->F for /ndn. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-e.sock prefixreg /ndn $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml

echo "Create forwarding rule E->F for /node/nodeF. FaceID: $FACEID"
$CCNL_HOME/bin/ccn-lite-ctrl -x /tmp/mgmt-nfn-relay-e.sock prefixreg /node/nodeF $FACEID ndn2013 \
  | $CCNL_HOME/bin/ccn-lite-ccnb2xml