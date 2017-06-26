cmake_minimum_required(VERSION 3.8)
project(ccnl-addons)

set(CCNLCFLAGS "-DCCNL_UNIX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g ${CCNLCFLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(include)
 
file(GLOB SOURCES "src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
