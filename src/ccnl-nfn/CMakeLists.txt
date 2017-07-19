cmake_minimum_required(VERSION 3.7)
project(ccnl-nfn)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-core/include ../ccnl-unix/include ../ccnl-addons/include ../ccnl-pkt/include ../ccnl-fwd/include)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
target_link_libraries(ccnl-nfn ccnl-core ccnl-pkt)
