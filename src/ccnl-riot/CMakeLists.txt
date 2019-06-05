cmake_minimum_required(VERSION 2.8)
project(ccnl-riot)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include ../ccnl-core/include ../ccnl-pkt/include ../ccnl-fwd/include ../ccnl-unix/include ../ccnl-utils/include ../ccnl-cs/include)
 
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")
file(GLOB CORESOURCE "../ccnl-core/src/*.c")
file(GLOB FWDSOURCE "../ccnl-fwd/src/*.c")
file(GLOB PKTSOURCE "../ccnl-pkt/src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${CORESOURCE} ${FWDSOURCE} ${PKTSOURCE} ${HEADERS})

