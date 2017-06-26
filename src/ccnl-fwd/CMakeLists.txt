cmake_minimum_required(VERSION 3.8)
project(ccnl-fwd)
 
set(SUITEFLAGS "-DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC")
set(USEFLAGS "-DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING -DNEEDS_PREFIX_MATCHING")
set(CCNLCFLAGS "${SUITEFLAGS} ${USEFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g ${CCNLCFLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(include ../ccnl-addons/include ../ccnl-pkt/include ../ccnl-core/include ../ccnl-unix/include)
 
file(GLOB SOURCES "src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
