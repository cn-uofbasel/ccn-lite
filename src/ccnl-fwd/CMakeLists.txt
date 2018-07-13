cmake_minimum_required(VERSION 2.8)
project(ccnl-fwd)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(include ../ccnl-pkt/include ../ccnl-core/include ../ccnl-unix/include)
 
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
