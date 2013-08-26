#!/bin/sh

# test-frag-utils.sh - test the demo utilities mkF and deF

# start in the test/script directory

UTIL_HOME=../../util

INPUTFILE=$UTIL_HOME/ccn-lite-mkC.c

echo "** Creating a content object"
$UTIL_HOME/ccn-lite-mkC /this/is/a/test/name <$INPUTFILE >$$.ccnb
echo

echo "** Fragmenting:"
$UTIL_HOME/ccn-lite-mkF -f $$.frag $$.ccnb
echo

echo "** Reassembling:"
$UTIL_HOME/ccn-lite-deF -f $$.defrag $$.frag00*.ccnb
echo

echo "Comparing:"
cmp -l $$.defrag000.ccnb $$.ccnb
if [ $? -eq 0 ]; then
  echo "  ok"
fi
echo

echo "After inspection of intermediate files, clean them up with \"rm $$.*\""

# eof
