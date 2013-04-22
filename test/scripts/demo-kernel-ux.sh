#!/bin/bash

# demo-kernel-ux.sh -- test/demo for ccn-lite:
# CCNx relaying via UNIX sockets, where the CCN relay sits in the Linux kernel

# execute with sudo!

CCNL_HOME=../..

UXA=/tmp/ccn-lite-lnxmodule-$$-a.sock
UXB=/tmp/ccn-lite-relay-$$-b.sock

# topology:

#  ccn-lite-peek --> kernel-relay A   -->  relay B
#
#                   /tmp/a.sock       /tmp/b.sock (unix sockets, ctrl+data)

# ----------------------------------------------------------------------

echo killing all ccn-lite relay instances:
killall ccn-lite-relay 2>/dev/null
rmmod ccn-lite-lnxkernel 2>/dev/null

# starting (kernel) relay A, with a link to relay B
insmod $CCNL_HOME/ccn-lite-lnxkernel.ko v=99 x=$UXA
sleep 1
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA newUNIXface $UXB
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA prefixreg /ccnx/0.7.1/doc/technical 2
$CCNL_HOME/util/ccn-lite-ctrl -x $UXA debug dump

# starting relay B, with content loading
$CCNL_HOME/ccn-lite-relay -v 99 -x $UXB -d $CCNL_HOME/test/ccnb 2>/tmp/b.log &
sleep 1

# test case: ask relay A to deliver content that is hosted at relay B
$CCNL_HOME/util/ccn-lite-peek -x $UXA /ccnx/0.7.1/doc/technical/NameEnumerationProtocol.txt | $CCNL_HOME/util/ccn-lite-parse

# shutdown both relays
rmmod ccn-lite-lnxkernel
$CCNL_HOME/util/ccn-lite-ctrl -x $UXB debug dump+halt

# eof
