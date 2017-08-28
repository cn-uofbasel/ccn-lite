cmake_minimum_required(VERSION 3.7)
project(ccnl-core)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)


if(DEFINED CCNL_RIOT)
include_directories(include ../ccnl-riot/include)
endif()
 
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
