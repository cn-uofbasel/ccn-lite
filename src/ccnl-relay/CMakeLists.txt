cmake_minimum_required(VERSION 3.8)
project(ccn-lite-relay)

set(PROJECT_LINK_LIBS libccnl-addons.a libccnl-core.a libccnl-pkt.a libccnl-fwd.a libccnl-unix.a libccnl-nfn.a)
set(EXT_LINK_LIBS ssl crypto)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-addons/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-core/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-pkt/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-fwd/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-unix/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-nfn/lib
)

include_directories(
	../ccnl-addons/include 
	../ccnl-pkt/include 
	../ccnl-fwd/include 
	../ccnl-core/include 
	../ccnl-unix/include
    ../ccnl-nfn/include
)
 
file(GLOB SOURCES "*.c")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS} ${EXT_LINK_LIBS})
