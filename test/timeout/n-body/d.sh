#!/bin/sh

ulimit -c unlimited
$CCNL_HOME/bin/ccn-nfn-relay -v trace -u 9004 -x /tmp/mgmt-nfn-relay-d.sock -d /Users/Bazsi/Uni/Thesis/Test/nbody-render/d-content/ndntlv
