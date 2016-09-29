#!/bin/sh

#java -jar "$NFN_HOME" --mgmtsocket /tmp/mgmt-nfn-relay-d.sock \
#  --ccnl-port 9004 --cs-port 9999 --debug --ccnl-already-running /node/nodeA

cd "$NFN_HOME"
sbt 'runMain runnables.production.ComputeServerStarter --mgmtsocket /tmp/mgmt-nfn-relay-d.sock --ccnl-port 9004 --cs-port 9999 --debug --ccnl-already-running /node/nodeA'