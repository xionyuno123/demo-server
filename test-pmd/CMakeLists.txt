cmake_minimum_required(VERSION 3.2)

project(test-pmd)

# search for installed dpdk
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk=20.11.4)

# search for other required libraries
find_library(HTTPD microhttpd REQUIRED)
find_library(JSON jansson REQUIRED)


# The cpp library
add_library(cpp_lib
  cppsrc/fill_buf.cc
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3")
foreach(entry ${DPDK_CFLAGS_OTHER})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${entry}")
endforeach()

target_include_directories(
  cpp_lib
  PUBLIC 
  ${PROJECT_SOURCE_DIR}/include
)
foreach(entry ${DPDK_INCLUDE_DIRS})
  target_include_directories(cpp_lib PUBLIC ${entry})
endforeach()

# The C executable
add_executable(${PROJECT_NAME} 
  csrc/5tswap.c
  csrc/cmdline.c
  csrc/cmdline_flow.c
  csrc/cmdline_mtr.c
  csrc/cmdline_tm.c
  csrc/bpf_cmd.c
  csrc/config.c
  csrc/csumonly.c
  csrc/flowgen.c
  csrc/icmpecho.c
  csrc/ieee1588fwd.c
  csrc/iofwd.c
  csrc/macfwd.c
  csrc/macswap.c
  csrc/noisy_vnf.c
  csrc/parameters.c
  csrc/rxonly.c
  csrc/testpmd.c
  csrc/txonly.c
  csrc/shunt.c
  csrc/aggre.c
  csrc/httpserver.c
  csrc/util.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
foreach(entry ${DPDK_CFLAGS_OTHER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${entry}")
endforeach()

target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC 
  ${PROJECT_SOURCE_DIR}/include
)
foreach(entry ${DPDK_INCLUDE_DIRS})
  target_include_directories(${PROJECT_NAME} PUBLIC ${entry})
endforeach()

foreach(entry ${DPDK_STATIC_LDFLAGS})
  target_link_libraries(${PROJECT_NAME} ${entry})
endforeach()
target_link_libraries(${PROJECT_NAME} ${HTTPD})
target_link_libraries(${PROJECT_NAME} ${JSON})
target_link_libraries(${PROJECT_NAME} cpp_lib)
