#!/bin/sh

#java -jar nfn.jar --mgmtsocket /tmp/mgmt-nfn-relay-d.sock \
#  --ccnl-port 9004 --cs-port 9999 --debug --ccnl-already-running /node/nodeA

cd /Users/Bazsi/Uni/Thesis/Code/nfn-scala
sbt 'runMain runnables.production.ComputeServerStarter --mgmtsocket /tmp/mgmt-nfn-relay-f.sock --ccnl-port 9006 --cs-port 9996 --debug --ccnl-already-running /node/nodeF'