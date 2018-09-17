cmake_minimum_required(VERSION 2.8)
project(ccn-lite-utils)

set(PROJECT_LINK_LIBS libccnl-core.a libccnl-pkt.a libccnl-fwd.a libccnl-unix.a libccnl-cs.a)
set(EXT_LINK_LIBS ssl crypto)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

link_directories(
    ${CMAKE_BINARY_DIR}/lib
)

# set include directories
include_directories(include ../ccnl-pkt/include ../ccnl-fwd/include ../ccnl-core/include ../ccnl-unix/include ../ccnl-cs/include)

add_library(common STATIC src/ccnl-common.c src/base64.c src/ccnl-socket.c)
add_library(ccnl-crypto STATIC src/ccnl-crypto.c src/ccnl-ext-hmac.c src/lib-sha256.c)

add_executable(ccn-lite-peek src/ccn-lite-peek.c)
#add_executable(ccn-lite-peekcomputation ccn-lite-peekcomputation.c) #todo work to do
add_executable(ccn-lite-ctrl src/ccn-lite-ctrl.c)
add_executable(ccn-lite-fetch src/ccn-lite-fetch.c) 
if(OpenSSL_FOUND)
    add_executable(ccn-lite-ccnb2xml src/ccn-lite-ccnb2xml.c)
    add_executable(ccn-lite-cryptoserver src/ccn-lite-cryptoserver.c)
    #add_executable(ccn-lite-deF ccn-lite-deF.c)
    add_executable(ccn-lite-mkC src/ccn-lite-mkC.c)
    add_executable(ccn-lite-valid src/ccn-lite-valid.c)
    add_executable(ccn-lite-rpc src/ccn-lite-rpc.c)
endif()

if(USE_FRAG)
    add_executable(ccn-lite-mkF src/ccn-lite-mkF.c)
endif()
add_executable(ccn-lite-mkI src/ccn-lite-mkI.c)
add_executable(ccn-lite-pktdump src/ccn-lite-pktdump.c)
add_executable(ccn-lite-produce src/ccn-lite-produce.c)

target_link_libraries(ccn-lite-peek ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-peek ccnl-core ccnl-pkt ccnl-fwd ccnl-unix common)

#target_link_libraries(ccn-lite-peekcomputation ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})#todo work to do
#target_link_libraries(ccn-lite-peekcomputation ccnl-core ccnl-pkt ccnl-fwd ccnl-unix)

target_link_libraries(ccn-lite-ctrl ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-ctrl ccnl-core ccnl-pkt ccnl-fwd ccnl-unix)

if(OpenSSL_FOUND)
    target_link_libraries(ccn-lite-ccnb2xml ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-ccnb2xml ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs ${OPENSSL_LIBRARIES} common)

    target_link_libraries(ccn-lite-cryptoserver ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-cryptoserver ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs ${OPENSSL_LIBRARIES} common)

    #target_link_libraries(ccn-lite-deF ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    #target_link_libraries(ccn-lite-deF ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-mkC ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES} common)
    target_link_libraries(ccn-lite-mkC ccnl-core ccnl-pkt ccnl-fwd ccnl-unix  ccnl-crypto ccnl-cs ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-valid ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES} common ccnl-crypto)
    target_link_libraries(ccn-lite-valid ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-rpc ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES} common)
    target_link_libraries(ccn-lite-rpc ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs ${OPENSSL_LIBRARIES})

endif()

target_link_libraries(ccn-lite-fetch ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-fetch ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs common)

if(USE_FRAG)
    target_link_libraries(ccn-lite-mkF ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
    target_link_libraries(ccn-lite-mkF ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs common)
endif()

target_link_libraries(ccn-lite-mkI ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-mkI ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs common)

target_link_libraries(ccn-lite-pktdump ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-pktdump ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs common)

target_link_libraries(ccn-lite-produce ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-produce ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-cs common ccnl-crypto)

