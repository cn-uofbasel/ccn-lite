cmake_minimum_required(VERSION 3.8)
project(ccn-lite-relay)

set(SUITEFLAGS "-DUSE_SUITE_CCNTLV -DUSE_SUITE_NDNTLV -DUSE_SUITE_CCNB -DUSE_SUITE_CISTLV -DUSE_SUITE_IOTTLV -DUSE_SUITE_LOCALRPC")
set(USEFLAGS "-DCCNL_UNIX -DUSE_IPV4 -DNEEDS_PACKET_CRAFTING -DNEEDS_PREFIX_MATCHING")
set(CCNLCFLAGS "${SUITEFLAGS} ${USEFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c99 -g ${CCNLCFLAGS}")
 
set(PROJECT_LINK_LIBS libccnl-addons.a libccnl-core.a libccnl-pkt.a libccnl-fwd.a libccnl-unix.a)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-addons/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-core/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-pkt/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-fwd/lib
	${CMAKE_CURRENT_SOURCE_DIR}/../ccnl-unix/lib
)

include_directories(
	../ccnl-addons/include 
	../ccnl-pkt/include 
	../ccnl-fwd/include 
	../ccnl-core/include 
	../ccnl-unix/include
)
 
file(GLOB SOURCES "*.c")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS})
