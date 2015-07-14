#!/bin/bash

# nfn-test.sh -- test/demo for ccn-lite with NFN
# Environment variables
SUITES=("ccnb" "ccnx2015" "cisco2015" "iot2014" "ndn2013")
USAGE="usage: nfn-test.sh [-v] SUITE \nwhere\n  SUITE =   ${SUITES[@]}\n  -v        enable verbose output"
RC=0
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
VERBOSE=0

# Load utils
source "$DIR/../utils.sh"

# Check parameters
while getopts "v" opt; do
    case $opt in
        v) VERBOSE=1; shift;
    esac
done

if [ "$#" -lt 1 ]; then
    echo "Error: missing parameter SUITE" 1>&2
    echo -e "$USAGE"
    exit 1
fi

# Check SUITE
SUITE=$1
if ! containsElement "$SUITE" "${SUITES[@]}"; then
    echo "Error: unknown SUITE '$SUITE'" 1>&2
    echo -e "$USAGE"
    exit 1
fi

# Check CCNL_HOME and binaries
check-and-set-ccnl-home "$DIR/../../.."
check-ccn-nfn

# Set NFN_JAR after checking CCNL_HOME
NFN_JAR="$CCNL_HOME/test/scripts/nfn/nfn.jar"
check-nfn "$NFN_JAR"

# Start test
echo -n "Killing all CCN-lite relay and Java instances..."
killall ccn-lite-relay 2> /dev/null
killall ccn-nfn-relay 2> /dev/null
killall java 2> /dev/null
sleep 1
echo " Done!"

echo -n "Starting two CCN-lite relays..."
"$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9000 -x /tmp/mgmt1.sock &> "/tmp/nfn-test-$SUITE-relay0.log" &
"$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9001 -x /tmp/mgmt2.sock &> "/tmp/nfn-test-$SUITE-relay1.log" &
sleep 1
echo " Done!"

echo "Adding UPD interface..."
echo "$ ccn-lite-ctrl newUDPface any 127.0.0.1 9001 | ccn-lite-ccnb2xml"
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001  | "$CCNL_HOME/bin/ccn-lite-ccnb2xml"
sleep 1
echo " Done!"

echo "Registering prefix..."
echo "$ ccn-lite-ctrl prefixreg /nfn/node2 2 $SUITE | ccn-lite-ccnb2xml"
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock prefixreg /nfn/node2 2 "$SUITE" | "$CCNL_HOME/bin/ccn-lite-ccnb2xml"
sleep 1
echo " Done!"

echo -n "Starting NFN compute server..."
java -jar "$NFN_JAR" -s "$SUITE" -m /tmp/mgmt2.sock -o 9001 -p 9002 -d -r /nfn/node2 &> "/tmp/nfn-test-$SUITE-computserver.log" &
sleep 5
echo " Done!"

echo "$ $CCNL_HOME/bin/ccn-lite-simplenfn -s "$SUITE" -u '127.0.0.1/9000'" "'call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md' | $CCNL_HOME/bin/ccn-lite-pktdump -s $SUITE -f 2"
$CCNL_HOME/bin/ccn-lite-simplenfn -v info -s "$SUITE" -u '127.0.0.1/9000' 'call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md' > /tmp/nfn-test-$SUITE-simplefn.log
if [ 0 -ne $? ]; then RC=1; fi
RES=$($CCNL_HOME/bin/ccn-lite-pktdump -s "$SUITE" -f 2 < "/tmp/nfn-test-$SUITE-simplefn.log")
if [[ 0 -ne $RC ]] || [[ 3558 -ne $RES ]]; then
    RC=1
fi

echo "$RES"
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" &> /dev/null
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt2.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" &> /dev/null

# Verbose output
if [[ 0 -ne $VERBOSE ]] && [[ 0 -ne $RC ]]; then
    for f in "relay0" "relay1" "computserver"; do
        echo "$ cat /tmp/nfn-test-$SUITE-$f.log"
        cat "/tmp/nfn-test-$SUITE-$f.log"
        echo ""
    done
fi

# Result output
echo -n "Expected: 3558, result: $RES"
if [[ 0 -ne $RC ]]; then
    echo $' [\e[1;31mfailed\e[0;0m]'
else
    echo $' [\e[1;32mok\e[0;0m]'
fi
echo "See /tmp/nfn-test-$SUITE-*.log for more information."

killall ccn-lite-relay 2> /dev/null
killall ccn-nfn-relay  2> /dev/null
killall java  2> /dev/null

exit $RC
