cmake_minimum_required(VERSION 2.8)

project(ccnl-cs-tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/ccnl-cs)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)
include_directories(include ../../src/ccnl-cs/include)

add_executable(test_cs test_cs.c)
target_link_libraries(test_cs ccnl-cs cmocka)
target_link_libraries(test_cs ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_cs test_cs)

add_executable(test_cs_uthash test_cs_uthash.c)
target_link_libraries(test_cs_uthash ccnl-cs cmocka)
target_link_libraries(test_cs_uthash ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_cs_uthash test_cs_uthash)

add_executable(test_cs_helper test_cs_helper.c)
target_link_libraries(test_cs_helper ccnl-cs cmocka)
target_link_libraries(test_cs_helper ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_cs_helper test_cs_helper)
