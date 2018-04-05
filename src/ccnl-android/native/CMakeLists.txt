# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

set(CCNL_ANDROID_BASIC_FLAGS
        -DUSE_DEBUG
        #-DUSE_DEBUG_MALLOC
        -DUSE_LOGGING
        #    -DUSE_SIGNATURES
        -DUSE_DUP_CHECK
        #    -DUSE_ECHO
        #-DUSE_HMAC256
        -DUSE_STATS
        -DNEEDS_PACKET_CRAFTING
        -DNEEDS_PREFIX_MATCHING
        -DUSE_LINKLAYER
        -DCCNL_ANDROID
        -DCCNL_UNIX
        #-DUSE_CCNxDIGEST
        -DUSE_MGMT
        -DUSE_UNIXSOCKET
        -DUSE_IPV4
        #-DUSE_IPV6
        -DUSE_HTTP_STATUS
        CACHE PATH
        "basic build flags for CCN-lite Android"
        )
add_definitions(${CCNL_ANDROID_BASIC_FLAGS})


set(CCNL_PACKETFORMAT_FLAGS
        -DUSE_SUITE_NDNTLV
        -DUSE_SUITE_CCNB
        -DUSE_SUITE_CCNTLV
        -DUSE_SUITE_CISTLV
        -DUSE_SUITE_LOCALRPC
        CACHE PATH
        "packet format flags for CCN-lite"
        )
add_definitions(${CCNL_PACKETFORMAT_FLAGS})

find_package(OpenSSL)

include_directories(include ../../ccnl-core/include ../../ccnl-pkt/include ../../ccnl-fwd/include ../../ccnl-unix/include ../../ccnl-nfn/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")
file(GLOB CORESOURCE "../ccnl-core/src/*.c")
file(GLOB FWDSOURCE "../ccnl-fwd/src/*.c")
file(GLOB PKTSOURCE "../ccnl-pkt/src/*.c")
file(GLOB JNISOURCE "src/*.c")

file(GLOB HEADERS "include/*.h")
add_library(ccn-lite-android SHARED ${JNISOURCE} ${CORESOURCE} ${FWDSOURCE} ${PKTSOURCE} ${HEADERS})
target_link_libraries()
