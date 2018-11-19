cmake_minimum_required(VERSION 2.8)

project(ccnl-core-tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/ccnl-core)

set(CCNL_EXTRA_FLAGS
        -DUSE_IPV4
        -DUSE_IPV6
    )
add_definitions(${CCNL_EXTRA_FLAGS})

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)
include_directories(include ../../src/ccnl-pkt/include ../../src/ccnl-cs/include ../../src/ccnl-fwd/include ../../src/ccnl-core/include ../../src/ccnl-unix/include ../../src/ccnl-cs/include)

add_executable(test_interest test_interest.c)
target_link_libraries(test_interest ccnl-core ccnl-cs ccnl-pkt cmocka)
target_link_libraries(test_interest ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_interest test_interest)

add_executable(test_sockunion test_sockunion.c)
target_link_libraries(test_sockunion ccnl-core cmocka)
target_link_libraries(test_sockunion${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_sockunion test_sockunion)

add_executable(test_producer test_producer.c)
target_link_libraries(test_producer ccnl-core cmocka)
target_link_libraries(test_producer ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_producer test_producer)

add_executable(test_pkt-util test_pkt-util.c)
target_link_libraries(test_pkt-util ccnl-core ccnl-pkt ccnl-fwd cmocka)
target_link_libraries(test_pkt-util ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_pkt-util test_pkt-util)

add_executable(test_content test_content.c)
target_link_libraries(test_content ccnl-core ccnl-cs ccnl-pkt cmocka)
target_link_libraries(test_content ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_content test_content)

add_executable(test_prefix test_prefix.c)
target_link_libraries(test_prefix ccnl-core ccnl-fwd ccnl-pkt ccnl-unix ccnl-cs cmocka)
target_link_libraries(test_prefix ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
add_test(test_prefix test_prefix)

#add_executable(test_prefix_bug test_prefix_bug.c)
#target_link_libraries(test_prefix_bug ccnl-core ccnl-fwd ccnl-pkt ccnl-unix ccnl-cs cmocka)
#target_link_libraries(test_prefix_bug ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
#add_test(test_prefix_bug test_prefix_bug)
