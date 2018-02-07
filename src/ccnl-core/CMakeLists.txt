cmake_minimum_required(VERSION 2.8)
project(ccnl-core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-pkt/include ../ccnl-nfn/include ../ccnl-unix/include ../ccnl-fwd/include ../ccnl-test/include ../ccnl-pkt-compression/include)

if(DEFINED CCNL_RIOT)
    include_directories(include ../ccnl-riot/include)
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")
file(GLOB TEST "test/unit.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})


add_executable(ccnl_core_test_prefix_comp "test/ccnl_core_test_prefix_comp.c")
target_link_libraries(ccnl_core_test_prefix_comp ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})

add_test(NAME ccnl_core_test_prefix_comp COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ccnl_core_test_prefix_comp 0)
