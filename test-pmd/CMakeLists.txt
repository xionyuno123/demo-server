cmake_minimum_required(VERSION 3.2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/share/cmake-3.16/Modules")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/home/djp/tmp/dpdk-lib/lib/x86_64-linux-gnu/pkgconfig")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/share/cmake-3.16/Modules")

message(STATUS "${CMAKE_MODULE_PATH}")
message(STATUS "${CMAKE_PREFIX_PATH}")

project(test-pmd-cpp)

find_package(FindPkgConfig)
pkg_check_module(DPDK REQUIRED dpdk libdpdk)
message(STATUS "dpdk include dir: ${DPDK_INCLUDE_DIRS}")
message(STATUS "dpdk libs: ${DPDK_LIBRARIES}")
message(STATUS "dpdk compile options: ${DPDK_CFLAGS_OTHER}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} 
  src/5tswap.cc
  src/cmdline.cc
  src/cmdline_flow.cc
  src/cmdline_mtr.cc
  src/cmdline_tm.cc
  src/config.cc
  src/csumonly.cc
  src/flowgen.cc
  src/icmpecho.cc
  src/ieee1588fwd.cc
  src/iofwd.cc
  src/macfwd.cc
  src/macswap.cc
  src/noisy_vnf.cc
  src/parameters.cc
  src/rxonly.cc
  src/main.cc
  src/txonly.cc
  src/shunt.cc
  src/aggre.cc
  src/httpserver.cc
  src/util.cc
)

target_include_directories(${PROJECT_NAME} 
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)