cmake_minimum_required(VERSION 2.8)

project(ccnl-core-tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/ccnl-core)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)
include_directories(include ../../src/ccnl-pkt/include ../../src/ccnl-fwd/include ../../src/ccnl-core/include ../../src/ccnl-unix/include)

add_executable(test_interest test_interest.c)
target_link_libraries(test_interest ccnl-core cmocka)
add_test(test_interest test_interest)

add_executable(test_prefix test_prefix.c)
target_link_libraries(test_prefix ccnl-core ccnl-fwd ccnl-pkt ccnl-unix cmocka)
target_link_libraries(test_prefix ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_prefix test_prefix)
