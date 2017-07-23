cmake_minimum_required(VERSION 3.7)
project(ccn-lite)

# CCNL flags
set(CCNL_BASIC_FLAGS
    -DUSE_DEBUG
    -DUSE_LOGGING
    -DUSE_CCNxDIGEST
#    -DUSE_SIGNATURES
    -DUSE_DUP_CHECK
#    -DUSE_ECHO
    -DUSE_HMAC256
    -DUSE_IPV4
    -DUSE_IPV6
    -DUSE_MGMT
    -DUSE_STATS
    -DUSE_UNIXSOCKET
    -DNEEDS_PACKET_CRAFTING
    -DNEEDS_PREFIX_MATCHING
    CACHE PATH
    "basic build flags for CCN-lite"
)

add_definitions(${CCNL_BASIC_FLAGS})

# Platforms
set(CCNL_PLATFORM_FLAGS
    -DCCNL_UNIX
#    -DCCNL_ARDUINO
     CACHE PATH
    "platform flags for CCN-lite"
)
add_definitions(${CCNL_PLATFORM_FLAGS})

# Packet formats
set(CCNL_PACKETFORMAT_FLAGS
    -DUSE_SUITE_NDNTLV
    -DUSE_SUITE_CCNB
    -DUSE_SUITE_CCNTLV
    -DUSE_SUITE_CISTLV
    -DUSE_SUITE_IOTTLV
    -DUSE_SUITE_LOCALRPC
    CACHE PATH
    "packet format flags for CCN-lite"
)
add_definitions(${CCNL_PACKETFORMAT_FLAGS})

# NFN flags
set(CCNL_NFN_FLAGS
    -DUSE_NFN
    -DUSE_NFN_REQUESTS
#    -DUSE_NFN_NSTRANS
    CACHE PATH
    "NFN flags for CCN-lite"
)
add_definitions(${CCNL_NFN_FLAGS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g")

#add_subdirectory(ccnl-addons)
add_subdirectory(ccnl-core)
add_subdirectory(ccnl-pkt)
add_subdirectory(ccnl-fwd)
add_subdirectory(ccnl-unix)
add_subdirectory(ccnl-nfn)
add_subdirectory(ccnl-relay)
add_subdirectory(ccnl-utils)

add_dependencies(ccn-lite-relay ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn)

find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
endif()

# unused:
set(CCNL_DISABLED_FLAGS "USE_LINKLAYER USE_DEBUG USE_DEBUG_MALLOC USE_FRAG \
						USE_NACK USE_NFN USE_NFN_MONITOR USE_NFN_KEEPALIVE \
						USE_NFN_PULL USE_SCHEDULER USE_SIGNATURES -DUSE_HTTP_STATUS")
