cmake_minimum_required(VERSION 3.8)
project(ccn-lite)

# CCNL flags
add_definitions(
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
)

# Platforms
add_definitions(
    -DCCNL_UNIX
#    -DCCNL_ARDUINO
)

# Packet formats
add_definitions(
    -DUSE_SUITE_NDNTLV
    -DUSE_SUITE_CCNB
    -DUSE_SUITE_CCNTLV
    -DUSE_SUITE_CISTLV
    -DUSE_SUITE_IOTTLV
    -DUSE_SUITE_LOCALRPC
)

# NFN flags
add_definitions(
    -DUSE_NFN
#    -DUSE_NFN_NSTRANS
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g")

add_subdirectory(ccnl-addons)
add_subdirectory(ccnl-pkt)
add_subdirectory(ccnl-fwd)
add_subdirectory(ccnl-core)
add_subdirectory(ccnl-unix)
add_subdirectory(ccnl-nfn)
add_subdirectory(ccnl-relay)

add_dependencies(ccn-lite-relay ccnl-addons ccnl-pkt ccnl-fwd ccnl-core ccnl-unix ccnl-nfn)


# unused:
set(CCNL_DISABLED_FLAGS "USE_LINKLAYER USE_DEBUG USE_DEBUG_MALLOC USE_FRAG \
						USE_NACK USE_NFN USE_NFN_MONITOR USE_NFN_KEEPALIVE \
						USE_NFN_PULL USE_SCHEDULER USE_SIGNATURES -DUSE_HTTP_STATUS")
