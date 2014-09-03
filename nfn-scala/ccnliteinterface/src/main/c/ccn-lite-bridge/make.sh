#!/bin/sh

echo "Compile JNI native library for the CCNLite interface..."
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

if [ "$(uname)" == "Darwin" ]; then
    # Do something under Mac OS X platform
    gcc -dynamiclib -o libCCNLiteInterfaceCCNbJni.jnilib ./libCCNLiteInterfaceCCNbJni.c ./open_memstream.c -lcrypto -I$JAVA_HOME/include -I$JAVA_HOME/include/darwin -framework JavaVM
    if [ $? -eq 0 ]; then
        echo "compiled libCCNLiteInterface.jnilib for osx"
    else
        echo "error compiling libCCNLiteInterface"
        exit 1
    fi
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    # Do something under Linux platform
    gcc --std=c99 -fPIC -shared -o libCCNLiteInterfaceCCNbJni.so ./libCCNLiteInterfaceCCNbJni.c -lm -lcrypto -I$JAVA_HOME/include -I$JAVA_HOME/include/linux/
    if [ $? -eq 0 ]; then
        echo "compiled libCCNLiteInterface.so for linux"
    else
        echo "error compiling libCCNLiteInterface"
        exit 1
    fi
fi

