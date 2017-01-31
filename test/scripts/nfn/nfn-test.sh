#!/bin/bash

# nfn-test.sh -- test/demo for ccn-lite with NFN
# Environment variables
SUITES=("ccnb" "ccnx2015" "cisco2015" "iot2014" "ndn2013")
USAGE="usage: nfn-test.sh [-v] SUITE \nwhere\n  SUITE =   ${SUITES[@]}\n  -v        enable verbose output"
PID1=-1
PID2=-1
PID3=-1
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
echo -n "$(date "+[%F %T]") Starting two CCN-lite relays..."
"$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9000 -x /tmp/mgmt1.sock &> "/tmp/nfn-test-$SUITE-relay0.log" &
PID1=$!
"$CCNL_HOME/bin/ccn-nfn-relay" -v trace -u 9001 -x /tmp/mgmt2.sock &> "/tmp/nfn-test-$SUITE-relay1.log" &
PID2=$!
sleep 4
echo " Done!"

echo "$(date "+[%F %T]") Adding UPD interface..."
echo "$(date "+[%F %T]") $ ccn-lite-ctrl newUDPface any 127.0.0.1 9001 | ccn-lite-ccnb2xml"
FACEID=$("$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock newUDPface any 127.0.0.1 9001 | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" | grep FACEID | sed "s/[^0-9]//g")
echo "FACEID=$FACEID"
sleep 4
echo " Done!"

echo "$(date "+[%F %T]") Registering prefix..."
echo "$(date "+[%F %T]") $ ccn-lite-ctrl prefixreg /nfn/node2 $FACEID $SUITE | ccn-lite-ccnb2xml"
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock prefixreg /nfn/node2 "$FACEID" "$SUITE" | "$CCNL_HOME/bin/ccn-lite-ccnb2xml"
sleep 4
echo " Done!"

echo -n "$(date "+[%F %T]") Starting NFN compute server..."
java -jar "$NFN_JAR" -s "$SUITE" -m /tmp/mgmt2.sock -o 9001 -p 9002 -d -r /nfn/node2 &> "/tmp/nfn-test-$SUITE-computserver.log" &
PID3=$!
sleep 12
echo " Done!"

echo "$(date "+[%F %T]") $ $CCNL_HOME/bin/ccn-lite-peek -s "$SUITE" -u '127.0.0.1/9000'" "'call 2 %2Fnfn%2Fnode2%2Fnfn_service_WordCount %2Fnfn%2Fnode2%2Fdocs%2Ftutorial_md/NFN' | $CCNL_HOME/bin/ccn-lite-pktdump -s $SUITE -f 2"
$CCNL_HOME/bin/ccn-lite-simplenfn -v trace -s "$SUITE" -u '127.0.0.1/9000' 'call 2 /nfn/node2/nfn_service_WordCount /nfn/node2/docs/tutorial_md' > /tmp/nfn-test-$SUITE-simplefn.log
if [ 0 -ne $? ]; then RC=1; fi
RES=$($CCNL_HOME/bin/ccn-lite-pktdump -s "$SUITE" -f 2 < "/tmp/nfn-test-$SUITE-simplefn.log")
if [[ 0 -ne $RC ]] || [[ 3558 -ne $RES ]]; then
    RC=1
fi

echo "$(date "+[%F %T]") $RES"

# Stop relays and NFN
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt1.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" &> /dev/null
"$CCNL_HOME/bin/ccn-lite-ctrl" -x /tmp/mgmt2.sock debug halt | "$CCNL_HOME/bin/ccn-lite-ccnb2xml" &> /dev/null
if ps -p $PID1 > /dev/null; then
    kill $PID1 2> /dev/null
fi
if ps -p $PID2 > /dev/null; then
    kill $PID2 2> /dev/null
fi
if ps -p $PID3 > /dev/null; then
    kill $PID3 2> /dev/null
fi

# Verbose output
if [[ 0 -ne $VERBOSE ]] && [[ 0 -ne $RC ]]; then
    for f in "relay0" "relay1" "computserver"; do
        echo "$(date "+[%F %T]") $ cat /tmp/nfn-test-$SUITE-$f.log"
        cat "/tmp/nfn-test-$SUITE-$f.log"
        echo ""
    done
fi

# Result output
echo -n "$(date "+[%F %T]") Expected: 3558, result: $RES"
if [[ 0 -ne $RC ]]; then
    echo $' [\e[1;31mfailed\e[0;0m]'
else
    echo $' [\e[1;32mok\e[0;0m]'
fi
echo "$(date "+[%F %T]") See /tmp/nfn-test-$SUITE-*.log for more information."

exit $RC
