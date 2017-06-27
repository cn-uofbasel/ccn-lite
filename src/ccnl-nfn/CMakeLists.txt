cmake_minimum_required(VERSION 3.8)
project(ccnl-nfn)

#set(SUITEFLAGS "-DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC -DUSE_SUITE_NDNTLV")
#set(USEFLAGS "-DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING -DNEEDS_PREFIX_MATCHING")
set(NFNFLAGS "-DUSE_NFN -DNEEDS_PACKET_CRAFTING")
set(CCNLCFLAGS "${SUITEFLAGS} ${USEFLAGS} ${NFNFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g ${CCNLCFLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(include ../ccnl-core/include ../ccnl-unix/include ../ccnl-addons/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} include/ccnl-nfncommon.h include/ccnl-nfnkrivine.h include/ccnl-nfnmonitor.h include/ccnl-nfnops.h include/ccnl-nfnparse.h src/ccnl-nfnutil.c src/ccnl-nfnutil.c include/ccnl-nfnutil.h)
