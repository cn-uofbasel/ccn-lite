cmake_minimum_required(VERSION 3.8)
project(ccnl-core)
 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(include ../ccnl-addons/include ../ccnl-pkt/include)
 
file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
