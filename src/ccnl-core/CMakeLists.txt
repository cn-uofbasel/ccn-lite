cmake_minimum_required(VERSION 3.7)
project(ccnl-core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-pkt/include ../ccnl-nfn/include ../ccnl-unix/include ../ccnl-fwd/include ../ccnl-pkt-compression/include)

if(DEFINED CCNL_RIOT)
    include_directories(include ../ccnl-riot/include)
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})