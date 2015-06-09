#!/bin/bash

LOGFNAME=$1
shift

make -k $@ >$LOGFNAME.log 2>&1
if [ $? = 0 ]; then
    echo -e "[\e[32mok\e[0m]      $LOGFNAME"
else
    echo -e "[\e[31mfailed\e[0m]  $LOGFNAME"
fi
