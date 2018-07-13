#!/bin/bash

# check if scan-build is in path
if ! hash scan-build 2>/dev/null
then
    echo "'scan-build' was not found in PATH"
else
mkdir -p build \
	&& cd build \
	&& scan-build -o scanbuildout cmake .. \
	&& scan-build -o scanbuildout make
fi
