cmake_minimum_required(VERSION 2.8)
project(ccnl-pkt)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include ../ccnl-core/include ../ccnl-fwd/include ../ccnl-cs/include)
 
set(SOURCES
  "src/ccnl-pkt-builder.c"
  "src/ccnl-pkt-switch.c"
)
if (CCNL_PACKETFORMAT_NDN)
  set(SOURCES "${SOURCES}" "src/ccnl-pkt-ndntlv.c")
endif ()
if (CCNL_PACKETFORMAT_CCNTLV)
  set(SOURCES "${SOURCES}" "src/ccnl-pkt-ccntlv.c")
endif ()
if (CCNL_PACKETFORMAT_CCNB)
  set(SOURCES "${SOURCES}" "src/ccnl-pkt-ccnb.c")
endif ()
if (CCNL_PACKETFORMAT_LOCALRPC)
  set(SOURCES "${SOURCES}" "src/ccnl-pkt-localrpc.c")
endif ()

add_library(${PROJECT_NAME} STATIC ${SOURCES})
