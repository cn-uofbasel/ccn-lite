#!/bin/sh

# if [[ ! -z "$NFN_SCALA" ]] then
if [ -z "$NFN_SCALA" ]; then
    echo "\$NFN_SCALA must be set to .../ccn-lite/nfn"
    exit
fi
# For OSX
export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:$NFN_SCALA/ccnliteinterface/src/main/c/ccn-lite-bridge"
# For linux
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$NFN_SCALA/ccnliteinterface/src/main/c/ccn-lite-bridge"

# TODO:
# - check if jnilib exists in lib path
# - compile lib if not exists

sbt



# 1. sudo apt-get install openjdk-7-jdk
# 2. sudo apt-get install libssl-dev
# 3. http://www.scala-sbt.org/0.13.2/docs/Getting-Started/Setup.html
# 4. JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
# 5. gcc --std=c99 -fPIC -shared -o libCCNLiteInterface.so ./libCCNLiteInterface.c -lm -lcrypto -I$JAVA_HOME/include -I$JAVA_HOME/include/linux/

# 6. cd ccn-lite:
# * EXTLIB: -lrt
# * make
# 7. cd nfn: ./sbt -Djava.library.path=/Users/basil/Dropbox/uni/master_thesis/code/ccnliteinterface/src/main/c/ccn-lite-bridge
