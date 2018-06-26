cmake_minimum_required(VERSION 2.8)
project(ccnl-core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)

include_directories(include ../ccnl-pkt/include ../ccnl-unix/include ../ccnl-fwd/include ../ccnl-test/include ../ccnl-utils/include)

if(DEFINED CCNL_RIOT)
    include_directories(include ../ccnl-riot/include)
endif()

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "include/*.h")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

if (BUILD_TESTING)
    file(GLOB TEST_SRC "test/*.c")
    foreach (TESTFILE ${TEST_SRC})
        get_filename_component(TEST_NAME ${TESTFILE} NAME_WE)

        add_executable(${TEST_NAME} "test/${TEST_NAME}.c")
        target_link_libraries(${TEST_NAME} ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ${OPENSSL_LIBRARIES})
        add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} 0)
    endforeach ()

endif()




