#!/bin/sh

# openbsd 4.9
# gcc 4.2.1
# openjdk 1.7.0

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
rm ./*.class libCCNLiteInterface.jnilib

scalac ../../scala/CCNLiteInterface.scala

if [ $? -eq 0 ]; then
    echo "compiled CCNLiteInterface"
else
    echo "error compiling CCLLiteInterface"
    exit 1
fi

# mv ../../scala/*.class .

# scalah CCNLiteInterface
# if [ $? -eq 0 ]; then
#     echo "created CCNLiteInterface.h"
# else
#     echo "could not create CCLLiteInterface.h"
#     exit 2
# fi

gcc -dynamiclib -o libCCNLiteInterface.jnilib ./libCCNLiteInterface.c ./open_memstream.c -lcrypto -I$JAVA_HOME/include -I$JAVA_HOME/include/darwin -framework JavaVM

if [ $? -eq 0 ]; then
    echo "compiled libCCNLiteInterface.jnilib"
else
    echo "error compiling libCCNLiteInterface.jnilib"
    exit 3
fi

scala -Djava.library.path=. CCNLiteInterface
if [ $? -eq 0 ]; then
    echo "executed CCNLiteInterface"
else
    echo "error executing CCNLiteInterface"
    exit 4
fi
