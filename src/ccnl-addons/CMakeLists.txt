cmake_minimum_required(VERSION 3.8)
project(ccnl-addons)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g ${CCNL_C_FLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})


