cmake_minimum_required(VERSION 2.8)
project(ccnl-core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-pkt/include ../ccnl-nfn/include ../ccnl-unix/include ../ccnl-fwd/include ../ccnl-test/include ../ccnl-pkt-compression/include)

if(DEFINED CCNL_RIOT)
    include_directories(include ../ccnl-riot/include)
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

if (BUILD_TESTING)
    add_executable(ccnl_core_test_prefix_comp_exact "test/ccnl_core_test_prefix_comp_exact.c")
    target_link_libraries(ccnl_core_test_prefix_comp_exact ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})
    add_test(NAME ccnl_core_test_prefix_comp_exact COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ccnl_core_test_prefix_comp_exact 0)

    add_executable(ccnl_core_test_prefix_comp_exact_no_match "test/ccnl_core_test_prefix_comp_exact_no_match.c")
    target_link_libraries(ccnl_core_test_prefix_comp_exact_no_match ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})
    add_test(NAME ccnl_core_test_prefix_comp_exact_no_match COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ccnl_core_test_prefix_comp_exact_no_match 0)
endif()




