cmake_minimum_required(VERSION 2.8)
project(ccnl-pkt)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include ../ccnl-core/include ../ccnl-fwd/include)

set(SOURCES
    "src/ccnl-pkt-builder.c"
    "src/ccnl-pkt-switch.c"
)
set(HEADERS
    "include/ccnl-pkt-builder.h"
    "include/ccnl-pkt-switch.h"
)
if (CCNL_PACKETFORMAT_NDN)
   set(SOURCES "${SOURCES}" "src/ccnl-pkt-ndntlv.c")
   set(HEADERS "${HEADERS}" "include/ccnl-pkt-ndntlv.h")
endif ()
if (CCNL_PACKETFORMAT_CCNTLV)
   set(SOURCES "${SOURCES}" "src/ccnl-pkt-ccntlv.c")
   set(HEADERS "${HEADERS}" "include/ccnl-pkt-ccntlv.h")
endif ()
if (CCNL_PACKETFORMAT_CCNB)
   set(SOURCES "${SOURCES}" "src/ccnl-pkt-ccnb.c")
   set(HEADERS "${HEADERS}" "include/ccnl-pkt-ccnb.h")
endif ()
if (CCNL_PACKETFORMAT_LOCALRPC)
   set(SOURCES "${SOURCES}" "src/ccnl-pkt-localrpc.c")
   set(HEADERS "${HEADERS}" "include/ccnl-pkt-localrpc.h")
endif ()

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
