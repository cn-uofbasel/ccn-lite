#!/bin/bash

# A helper script to build specific targets.
# It prints either 'ok', 'warning' or 'failed' depending on the outcome of the build.
# The output of the build is stored in /tmp/$LOG_FNAME. If the build failed, the
# modified file $TARGET_FNAME is stored in /tmp/$TARGET_FNAME.$LOG_FNAME.
#
# Parameters (passed in environment variables):
# 	LOG_FNAME	name of the logfile to write to
#	MAKE_TARGETS	targets that need to be built
#	MAKE_VARS	variable-value pairs that are sent to the Makefile
#	MODIFIY_FNAME	name of the file to change #defines
#	SET_VARS	#define variables that need to be defined
#	UNSET_VARS	#define variables that need to be unset

# Undefining all environment variables in this invocation (build variables are passed as MAKE_VARS)
unset USE_KRNL
unset USE_FRAG
unset USE_NFN
unset USE_SIGNATURES

if [ -n "$MODIFIY_FNAME" ]; then
    # Backup
    cp "$MODIFIY_FNAME" "$MODIFIY_FNAME.bak"

    # Define variables that are commented out
    for VAR in $SET_VARS; do
        #  echo "Defining $VAR..."
        sed -i "s!^\s*//\s*#define $VAR!#define $VAR!" "$MODIFIY_FNAME"
    done

    # Comment already defined variables
    for VAR in $UNSET_VARS; do
        #  echo "Unsetting $VAR..."
        sed -i "s!^\s*#define $VAR!// #define $VAR!" "$MODIFIY_FNAME"
    done
fi

printf "%-30s [..]" "$LOG_FNAME"

# Build and log output
make -k $MAKE_VARS $MAKE_TARGETS > "/tmp/$LOG_FNAME.log" 2>&1

# Print status
if [ $? = 0 ]; then
    if ! grep --quiet -i "warning" "/tmp/$LOG_FNAME.log"; then
        echo -e "\b\b\b\b[\e[1;92mok\e[0;0m]"
    else
        echo -e "\b\b\b\b\b\b\b\b\b[\e[1;93mwarning\e[0;0m]"
    fi
else
    echo -e "\b\b\b\b\b\b\b\b[\e[1;91mfailed\e[0;0m]"
    if [ -n "$MODIFIY_FNAME" ]; then
        cp "$MODIFIY_FNAME" "/tmp/$MODIFIY_FNAME.$LOG_FNAME"
    fi
fi

# Replace backup
if [ -n "$MODIFIY_FNAME" ]; then
    mv "$MODIFIY_FNAME.bak" "$MODIFIY_FNAME"
fi
