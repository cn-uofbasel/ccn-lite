cmake_minimum_required(VERSION 3.8)
project(ccnl-nfn)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(include ../ccnl-core/include ../ccnl-unix/include ../ccnl-addons/include ../ccnl-pkt/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} include/ccnl-nfncommon.h include/ccnl-nfnkrivine.h include/ccnl-nfnmonitor.h include/ccnl-nfnops.h include/ccnl-nfnparse.h src/ccnl-nfnutil.c src/ccnl-nfnutil.c include/ccnl-nfnutil.h)
