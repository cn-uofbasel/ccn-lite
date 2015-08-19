#!/bin/bash
print-back() {
    local n=$1
    for i in $(seq 1 $n); do
        printf '\b'
    done
}

containsElement() {
    local e
    for e in "${@:2}"; do
        [[ "$e" == "$1" ]] && return 0
    done
    return 1
}

check-binary-exists() {
    local binary=$1
    local errorMsg=$2

    if [ ! -f "$CCNL_HOME/bin/$binary" ]; then
        echo "Error: $errorMsg"
        exit 1
    fi
}

check-and-set-ccnl-home() {
    local ccnlHome=$1

    if [ -z "$CCNL_HOME" ]
    then
        echo "\$CCNL_HOME is not set. Setting it to: '$ccnlHome'"
        export CCNL_HOME="$ccnlHome"
    fi
}

check-ccn-lite() {
    local relay="ccn-lite-relay"
    check-binary-exists "$relay" "Cannot find binary '$relay', please run 'make clean all' in \$CCNL_HOME/src."
}

check-ccn-nfn() {
    local relay="ccn-nfn-relay"
    check-binary-exists "$relay" "Cannot find binary '$relay', please run 'make clean all USE_NFN=1' in \$CCNL_HOME/src."
}

check-nfn() {
    local nfnFile="$1"

    if [ ! -f "$nfnFile" ]; then
        printf "%-47s [..]" "Info: cannot find NFN binary, downloading"
        wget -O "$nfnFile" https://github.com/cn-uofbasel/nfn-scala/releases/download/v0.2.0/nfn-assembly-0.2.0.jar > /tmp/nfn-download-jar.log 2>&1
        if [ $? -ne 0 ]; then
            print-back 8
            echo $'[\e[1;31mfailed\e[0;0m]'
            echo "\$ cat /tmp/nfn-download-jar.log"
            cat /tmp/nfn-download-jar.log
            echo ""
            echo "Please download the NFN binary from 'https://github.com/cn-uofbasel/nfn-scala/releases' and save it in '\$CCNL_HOME/test/scripts/nfn'"
            exit 1
        else
            print-back 6
            echo $'[\e[1;32mdone\e[0;0m]'
        fi
    fi
}
