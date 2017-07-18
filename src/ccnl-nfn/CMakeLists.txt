cmake_minimum_required(VERSION 3.7)
project(ccnl-nfn)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

include_directories(include ../ccnl-core/include ../ccnl-unix/include ../ccnl-addons/include ../ccnl-pkt/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} include/ccnl-nfn-common.h include/ccnl-nfn-krivine.h include/ccnl-nfn-monitor.h include/ccnl-nfn-ops.h include/ccnl-nfn-parse.h src/ccnl-nfn-util.c src/ccnl-nfn-util.c include/ccnl-nfn-util.h)
