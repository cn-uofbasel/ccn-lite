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
    #test exact prefix matching
    set(TEST_NAME ccnl_core_test_prefix_comp_exact_match)
    add_executable(${TEST_NAME} "test/${TEST_NAME}.c")
    target_link_libraries(${TEST_NAME} ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})
    add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} 0)

    #test exact prefix no match
    set(TEST_NAME ccnl_core_test_prefix_comp_exact_no_match)
    add_executable(${TEST_NAME} "test/${TEST_NAME}.c")
    target_link_libraries(${TEST_NAME} ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})
    add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} 0)

    #test longest  prefix matching
    set(TEST_NAME ccnl_core_test_prefix_comp_longest_match)
    add_executable(${TEST_NAME} "test/${TEST_NAME}.c")
    target_link_libraries(${TEST_NAME} ccnl-core ccnl-pkt ccnl-fwd ccnl-nfn ccnl-unix ${OPENSSL_LIBRARIES})
    add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} 0)
endif()




