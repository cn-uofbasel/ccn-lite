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

