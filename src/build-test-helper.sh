#!/bin/bash

# A helper script to execute all build-test commands.
#
# Parameters (passed in environment variables):
#   TARGET  Name of the target. Also used for the log file name.
#   MODE    Executes different commands for each mode: "make", "pkt-format" and "demo-relay". Each mode requires different variables.

# Modes:
#   "make"
#      Invokes the CCN-lite Makefile with the passed variables and targets
#      Parameters:
#        MAKE_TARGETS  Targets that need to be built.
#        MAKE_VARS     (optional) Variable-value pairs that are sent to the Makefile.
#        MODIFIY_FILE  (optional) Name of the file to change #defines.
#        SET_VARS      (optional) #define variables that need to be defined.
#        UNSET_VARS    (optional) #define variables that need to be unset.
#
#   "pkt-format"
#      Executes the packet format tests by compiling ccn-lite-pktdump and let pktdump parse all available test packets from the specified packet format.
#      Parameters:
#        PKT_FORMAT    Name of the packet format to test.
#
#   "demo-relay"
#      Runs the demo-relay with the provided suite.
#      Parameters:
#        SUITE         Name of the suite to test.
#        WITH_KRNL     If "true", use the kernel version instead of the normal one.
#
#   "nfn-test"
#      Runs the nfn-test script with the provided suite.
#      Parameters:
#        SUITE         Name of the suite to test.


### Functions:

# Modifies the '#define' variables in a provided file:
#
# Parameters:
#     $1    filename of the file to modify
#     $2    list of variables that need to be set and are commented out
#     $3    list of variables that are set and need to be commented out
#
# This function modifies the passed file.
build-test-modify-defines() {
    local file=$1
    local setVariables=$2
    local unsetVariables=$3

    # Define variables that are commented out
    for var in $setVariables; do
        sed -e "s!^\s*//\s*#define $var!#define $var!" "$file" > "$file.sed"
        mv "$file.sed" "$file"
    done

    # Comment already defined variables
    for var in $unsetVariables; do
        sed -e "s!^\s*#define $var!// #define $var!" "$file" > "$file.sed"
        mv "$file.sed" "$file"
    done
}

# Builds ccn-lite and logs the output in a specified log file.
#
# Parameters:
#     $1    log file
#     $2... parameters passed to make
build-test-make() {
    # TODO: fix $NO_CORES!
    local logfile=$1; shift
    local rc=0

    echo "$ make clean USE_NFN=1 USE_NACK=1" >> "$logfile"
    make clean USE_NFN=1 USE_NACK=1 >> "$logfile" 2>&1
    echo "" >> "$logfile"

    echo "$ make -j$NO_CORES -k $@" >> "$logfile"
    make -j$NO_CORES -k -B $@ >> "$logfile" 2>&1
    rc=$?
    echo "" >> "$logfile"

    return $rc
}

# Tests a specific packet format by feeding all available test files to
# ccn-lite-pktdump.
#
# Parameters:
#     $1    log file
#     $2    packet format
build-test-packet-format() {
    local logfile=$1;
    local pktFormat=$2;

    echo "$ make -j$NO_CORES -C util ccn-lite-pktdump" >> "$logfile"
    make -j$NO_CORES -C util ccn-lite-pktdump >> "$logfile"
    if [ $? -ne 0 ]; then
        return 1
    fi
    echo "" >> "$logfile"

    local rc=0
    local files=""

    files=$(find ../test/$pktFormat -iname "*.$pktFormat")
    for file in $files; do
        echo "$ ccn-lite-pktdump < $file" >> "$logfile"
        ./util/ccn-lite-pktdump < $file >> "$logfile" 2>&1
        if [ $? -ne 0 ]; then
            rc=1
        fi
        echo "" >> "$logfile"
    done

    return $rc
}

# Runs the demo-relay.sh script.
#
# Parameters:
#     $1    log file
#     $2    suite
#     $3    relay mode (ux or udp)
#     $4    use kernel ("true" or "false")
build-test-demo-relay() {
    local logfile=$1
    local suite=$2
    local relayMode=$3
    local useKernel=$4
    local rc

    echo "$ ../test/scripts/demo-relay.sh $suite $relayMode $useKernel" >> "$logfile"
    ../test/scripts/demo-relay.sh "$suite" "$relayMode" "$useKernel" >> "$logfile" 2>&1
    rc=$?
    echo "" >> "$logfile"

    return $rc
}

# Runs the nfn-test.sh script.
#
# Parameters:
#     $1    log file
#     $2    suite
build-test-nfn-test() {
    local logfile=$1
    local suite=$2
    local rc

    echo "$ ../test/scripts/nfn/nfn-test.sh -v $suite" >> "$logfile"
    ../test/scripts/nfn/nfn-test.sh -v "$suite" >> "$logfile" 2>&1
    rc=$?
    echo "" >> "$logfile"

    return $rc
}

### Main script:

unset USE_KRNL
unset USE_FRAG
unset USE_NFN
unset USE_SIGNATURES
LOGFILE="/tmp/$TARGET.log"
RC=0

printf "%-30s [..]" "$TARGET"

rm -f "$LOGFILE"

if [ "$MODE" = "make" ]; then

    if [ -n "$MODIFIY_FILE" ]; then
        cp "$MODIFIY_FILE" "$MODIFIY_FILE.bak"
        echo "Modifying $MODIFIY_FILE..." >> "$LOGFILE"
        build-test-modify-defines "$MODIFIY_FILE" "$SET_VARS" "$UNSET_VARS"
        if [ $? -ne 0 ]; then RC=1; fi
        echo "" >> "$LOGFILE"
    fi

    build-test-make "$LOGFILE" $MAKE_VARS $MAKE_TARGETS
    if [ $? -ne 0 ]; then RC=1; fi

    if [ -n "$MODIFIY_FILE" ]; then
        cp "$MODIFIY_FILE" "/tmp/$MODIFIY_FILE.$TARGET"
        mv "$MODIFIY_FILE.bak" "$MODIFIY_FILE"
    fi

elif [ "$MODE" = "pkt-format" ]; then

    build-test-packet-format "$LOGFILE" "$PKT_FORMAT"
    if [ $? -ne 0 ]; then RC=1; fi

elif [ "$MODE" = "demo-relay" ]; then

    if [ "$WITH_KRNL" = "true" ]; then
        MAKE_VARS="USE_KRNL=1"
    else
        MAKE_VARS=""
        WITH_KRNL="false"
    fi

    echo "$ make -j$NO_CORES all $MAKE_VARS" >> "$LOGFILE"
    make -j$NO_CORES all $MAKE_VARS >> "$LOGFILE"
    if [ $? -ne 0 ]; then
        RC=1
    else
        for M in "ux" "udp"; do
            build-test-demo-relay "$LOGFILE" "$SUITE" "$M" "$WITH_KRNL"
            if [ $? -ne 0 ]; then RC=1; fi
        done
    fi

elif [ "$MODE" = "nfn-test" ]; then

    echo "$ make -j$NO_CORES clean all USE_NFN=1" >> "$LOGFILE"
    make clean >> "$LOGFILE"
    make -j$NO_CORES all USE_NFN=1 >> "$LOGFILE"
    if [ $? -ne 0 ]; then
        RC=1
    else
        build-test-nfn-test "$LOGFILE" "$SUITE"
        RC=$?
    fi

else

    echo "Error! Unknown build-test-helper mode: '$MODE'" >> "$LOGFILE"
    RC=2

fi

if [ $RC -eq 0 ]; then
    if ! grep --quiet -i "warning" "$LOGFILE"; then
        echo $'\b\b\b\b[\e[1;32mok\e[0;0m]'
    else
        echo $'\b\b\b\b\b\b\b\b\b[\e[1;33mwarning\e[0;0m]'
    fi
else
    echo $'\b\b\b\b\b\b\b\b[\e[1;31mfailed\e[0;0m]'
fi
exit $RC
