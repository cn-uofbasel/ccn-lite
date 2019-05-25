cmake_minimum_required(VERSION 2.8)

project(ccnl-cs-tests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/ccnl-cs)

option(CCNL_PACKETFORMAT_NDN "Use the NDN (v0.2) packet parser." ON)

set(CCNL_PACKETFORMAT_FLAGS "")
if (CCNL_PACKETFORMAT_NDN)
   set(CCNL_PACKETFORMAT_FLAGS "${CCNL_PACKETFORMAT_FLAGS}" -DUSE_SUITE_NDNTLV)
endif ()
if (CCNL_PACKETFORMAT_CCNB)
   set(CCNL_PACKETFORMAT_FLAGS "${CCNL_PACKETFORMAT_FLAGS}" -DUSE_SUITE_CCNB)
endif ()
if (CCNL_PACKETFORMAT_CCNTLV)
   set(CCNL_PACKETFORMAT_FLAGS "${CCNL_PACKETFORMAT_FLAGS}" -DUSE_SUITE_CCNTLV)
endif ()
if (CCNL_PACKETFORMAT_LOCALRPC)
   set(CCNL_PACKETFORMAT_FLAGS "${CCNL_PACKETFORMAT_FLAGS}" -DUSE_SUITE_LOCALRPC)
endif ()
set("${CCNL_PACKETFORMAT_FLAGS}" CACHE PATH "packet format flags for CCN-lite")

add_definitions(${CCNL_PACKETFORMAT_FLAGS})

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)
include_directories(include ../../src/ccnl-cs/include ../../src/ccnl-core/include ../../src/ccnl-pkt/include)

#add_executable(test_cs test_cs.c)
#target_link_libraries(test_cs ccnl-cs cmocka)
#target_link_libraries(test_cs ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
#add_test(test_cs test_cs)

add_executable(test_cs_wip test_cs_wip.c)
target_link_libraries(test_cs_wip ccnl-core ccnl-pkt ccnl-fwd ccnl-cs cmocka)
target_link_libraries(test_cs_wip ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY} ccnl-core ccnl-pkt)
add_test(test_cs_wip test_cs_wip)

#add_executable(test_cs_uthash test_cs_uthash.c)
#target_link_libraries(test_cs_uthash ccnl-cs ccnl-core ccnl-pkt cmocka)
#target_link_libraries(test_cs_uthash ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
#add_test(test_cs_uthash test_cs_uthash)

#add_executable(test_cs_helper test_cs_helper.c)
#target_link_libraries(test_cs_helper ccnl-cs ccnl-core cmocka)
#target_link_libraries(test_cs_helper ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
#add_test(test_cs_helper test_cs_helper)
