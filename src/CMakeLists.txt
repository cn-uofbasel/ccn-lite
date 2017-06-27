cmake_minimum_required(VERSION 3.8)
project(ccn-lite)

set(CMAKE_SYSTEM_NAME Generic)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
# include(GNUInstallDirs)

include_directories(ccnl-addons/include)

# Include Boost as an imported target
#find_package(Boost REQUIRED)
#add_library(boost INTERFACE IMPORTED)
#set_property(TARGET boost PROPERTY
#    INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

# Some other library that we import that was also built using CMake
# and has an exported target.
#find_package(MyOtherLibrary REQUIRED)

# Targets that we develop here
#enable_testing()
add_subdirectory(ccnl-addons)
add_subdirectory(ccnl-pkt)
add_subdirectory(ccnl-fwd)
add_subdirectory(ccnl-core)
add_subdirectory(ccnl-unix)
add_subdirectory(ccnl-relay)



