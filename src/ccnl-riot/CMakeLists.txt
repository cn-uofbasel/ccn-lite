cmake_minimum_required(VERSION 3.7)
project(ccnl-riot)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-core/include ../ccnl-pkt/include ../ccnl-fwd/include ../ccnl-unix/include ../ccnl-nfn/include ../ccnl-pkt-compression/include)
 
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")
file(GLOB CORESOURCE "../ccnl-core/src/*.c")
file(GLOB FWDSOURCE "../ccnl-fwd/src/*.c")
file(GLOB PKTSOURCE "../ccnl-pkt/src/*.c")
file(GLOB PKTCOMPSOURCE "../ccnl-pkt-compression/src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${CORESOURCE} ${FWDSOURCE} ${PKTSOURCE} ${PKTCOMPSOURCE} ${HEADERS})

