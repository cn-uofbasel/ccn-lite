#!/bin/sh

# test-digest.sh -- test name matching with the implicit digest name field

. ./paths.sh

# topology:

#  ccn-lite-peek --> relay A
#
#                 127.0.0.1/9695

# ----------------------------------------------------------------------

echo killing all ccn-lite-relay instances ...
killall ccn-lite-relay

# starting relay A
$CCNL_HOME/ccn-lite-relay -v 99 -d tmp 2>/tmp/a.log &
sleep 1

# test case 1: ask relay A to deliver content, simple name
echo Test case 1/simple name: we should receive 221 output lines
$CCNL_HOME/util/ccn-lite-peek /ccnx/0.7.1/doc/technical/URI.txt | $CCNL_HOME/util/ccn-lite-pktdump | wc

# test case 2: ask relay A to deliver content, name includes correct digest
echo Test case 2/name with digest: we should receive 221 output lines
$CCNL_HOME/util/ccn-lite-peek /ccnx/0.7.1/doc/technical/URI.txt/%ed%4d%54%c3%e8%59%f2%3c%b6%1f%84%96%00%d2%bc%c1%5b%68%da%bd%81%a6%6a%5c%ce%53%ca%9a%0a%dc%1d%d6 | $CCNL_HOME/util/ccn-lite-pktdump | wc

exit 

# test case 3: ask relay A to deliver content, name includes wrong digest
echo Test case 3/name with invalid digest: timeout \(and a few error lines\)
$CCNL_HOME/util/ccn-lite-peek /ccnx/0.7.1/doc/technical/URI.txt/%ed%4d%54%c3%e8%59%f2%3c%b6%1f%84%96%00%d2%bc%c1%5b%68%da%bd%81%a6%6a%5c%ce%53%ca%9a%0a%dc%1d%d5 | $CCNL_HOME/util/ccn-lite-pktdump | wc

# eof
