cmake_minimum_required(VERSION 3.7)
project(ccn-lite-utils)

set(PROJECT_LINK_LIBS libccnl-core.a libccnl-pkt.a libccnl-fwd.a libccnl-unix.a libccnl-nfn.a)
set(EXT_LINK_LIBS ssl crypto)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../bin)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/../lib
)

include_directories(
    ../ccnl-pkt/include
    ../ccnl-fwd/include
    ../ccnl-core/include
    ../ccnl-unix/include
    ../ccnl-nfn/include
    ../ccnl-pkt-compression/include
    ${OPENSSL_INCLUDE_DIR}
    
)

find_package(OpenSSL REQUIRED)

add_executable(ccn-lite-peek ccn-lite-peek.c)
#add_executable(ccn-lite-peekcomputation ccn-lite-peekcomputation.c) #todo work to do
add_executable(ccn-lite-ctrl ccn-lite-ctrl.c)
add_executable(ccn-lite-ccnb2xml ccn-lite-ccnb2xml.c)
add_executable(ccn-lite-cryptoserver ccn-lite-cryptoserver.c)
#add_executable(ccn-lite-deF ccn-lite-deF.c)
add_executable(ccn-lite-fetch ccn-lite-fetch.c)
add_executable(ccn-lite-mkC ccn-lite-mkC.c)
add_executable(ccn-lite-valid ccn-lite-valid.c)
if(USE_FRAG)
    add_executable(ccn-lite-mkF ccn-lite-mkF.c)
endif()
add_executable(ccn-lite-mkI ccn-lite-mkI.c)
add_executable(ccn-lite-pktdump ccn-lite-pktdump.c)
add_executable(ccn-lite-produce ccn-lite-produce.c)
add_executable(ccn-lite-rpc ccn-lite-rpc.c)
add_executable(ccn-lite-simplenfn ccn-lite-simplenfn.c)


target_link_libraries(ccn-lite-peek ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-peek ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)

#target_link_libraries(ccn-lite-peekcomputation ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})#todo work to do
#target_link_libraries(ccn-lite-peekcomputation ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn)

target_link_libraries(ccn-lite-ctrl ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-ctrl ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)

if(OpenSSL_FOUND)
    target_link_libraries(ccn-lite-ccnb2xml ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-ccnb2xml ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-cryptoserver ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-cryptoserver ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression ${OPENSSL_LIBRARIES})
    
    #target_link_libraries(ccn-lite-deF ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    #target_link_libraries(ccn-lite-deF ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-mkC ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-mkC ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-valid ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-valid ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression ${OPENSSL_LIBRARIES})

    target_link_libraries(ccn-lite-rpc ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS} ${OPENSSL_LIBRARIES})
    target_link_libraries(ccn-lite-rpc ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression ${OPENSSL_LIBRARIES})

endif()

target_link_libraries(ccn-lite-fetch ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-fetch ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)

if(USE_FRAG)
    target_link_libraries(ccn-lite-mkF ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
    target_link_libraries(ccn-lite-mkF ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)
endif()

target_link_libraries(ccn-lite-mkI ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS}) 
target_link_libraries(ccn-lite-mkI ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression) 

target_link_libraries(ccn-lite-pktdump ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-pktdump ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)

target_link_libraries(ccn-lite-produce ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-produce ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)

target_link_libraries(ccn-lite-simplenfn ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
target_link_libraries(ccn-lite-simplenfn ccnl-core ccnl-pkt ccnl-fwd ccnl-unix ccnl-nfn ccnl-pkt-compression)




