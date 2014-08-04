#!/bin/sh

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

gcc -dynamiclib -o libCCNLiteInterface.jnilib ./libCCNLiteInterface.c ./open_memstream.c -lcrypto -I$JAVA_HOME/include -I$JAVA_HOME/include/darwin -framework JavaVM

if [ $? -eq 0 ]; then
    echo "compiled libCCNLiteInterface.jnilib"
else
    echo "error compiling libCCNLiteInterface.jnilib"
    exit 3
fi
