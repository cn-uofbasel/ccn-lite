cmake_minimum_required(VERSION 3.7)
project(ccn-lite-utils)

set(PROJECT_LINK_LIBS libccnl-core.a libccnl-pkt.a libccnl-fwd.a libccnl-unix.a libccnl-nfn.a)
set(EXT_LINK_LIBS ssl crypto)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../bin)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/../lib
)

include_directories(
    ../ccnl-pkt/include
    ../ccnl-fwd/include
    ../ccnl-core/include
    ../ccnl-unix/include
    ../ccnl-nfn/include
)

add_executable(ccn-lite-peek ccn-lite-peek.c)

target_link_libraries(ccn-lite-peek ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-peek ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn)
