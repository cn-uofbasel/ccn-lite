cmake_minimum_required(VERSION 2.8)
project(ccnl-core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include ../ccnl-pkt/include ../ccnl-unix/include ../ccnl-fwd/include ../ccnl-test/include ../ccnl-utils/include)

if(DEFINED CCNL_RIOT)
    include_directories(include ../ccnl-riot/include)
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})





