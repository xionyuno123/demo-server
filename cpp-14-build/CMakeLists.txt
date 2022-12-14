cmake_minimum_required(VERSION 3.2)

project(test-pmd-cpp)

# -include rte_config.h -march=native -I/home/djp/test/dpdk-install/include -I/usr/local/include -I/usr/include/libnl3
# -L/home/djp/test/dpdk-install/lib/x86_64-linux-gnu -Wl,--as-needed -lrte_node -lrte_graph -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_regexdev -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs

# Please specify the correct dpdk installation directory.
set(DPDK_INSTALL_PATH "/home/djp/tmp/dpdk-lib")

find_path(DPDK_INCLUDE_DIR rte_config.h
  HINTS ${DPDK_INSTALL_PATH}/include
  REQUIRED
  NO_DEFAULT_PATH)

# The following library list is reported by the pkg-config on installed libdpdk
set(components 
  node graph bpf flow_classify pipeline table port fib ipsec vhost stack security sched reorder rib rawdev pdump power member lpm latencystats kni jobstats ip_frag gso gro eventdev efd distributor cryptodev compressdev cfgfile bitratestats bbdev acl timer hash metrics cmdline pci ethdev meter net mbuf mempool rcu ring eal telemetry kvargs)

# foreach(c ${components})
#   find_library(DPDK_rte_${c}_LIBRARY rte_${c}
#     HINTS ${DPDK_INSTALL_PATH}/lib/x86_64-linux-gnu
#     REQUIRED
#     NO_DEFAULT_PATH)
# endforeach()

find_library(HTTPD microhttpd REQUIRED)
find_library(JSON jansson REQUIRED)

message(STATUS ${HTTPD})
message(STATUS ${JSON})

foreach(c ${components})
  list(APPEND check_LIBRARIES "${DPDK_rte_${c}_LIBRARY}")
endforeach()

list(APPEND check_LIBRARIES "${HTTPD}")
list(APPEND check_LIBRARIES "${JSON}")

# By marking the two variables as advanced, they will not appear in cmake output logs.
mark_as_advanced(DPDK_INCLUDE_DIR ${check_LIBRARIES})

set(DPDK_LIBRARIES
    -Wl,--whole-archive ${check_LIBRARIES} -lpthread -lnuma -ldl -lm -Wl,--no-whole-archive)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -march=native")

include_directories (${DPDK_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} 
  # src/5tswap.cc
  # src/cmdline.cc
  # src/cmdline_flow.cc
  # src/cmdline_mtr.cc
  # src/cmdline_tm.cc
  # src/config.cc
  # src/csumonly.cc
  # src/flowgen.cc
  # src/icmpecho.cc
  # src/ieee1588fwd.cc
  # src/iofwd.cc
  # src/macfwd.cc
  # src/macswap.cc
  # src/noisy_vnf.cc
  # src/parameters.cc
  # src/rxonly.cc
  # src/main.cc
  # src/txonly.cc
  # src/shunt.cc
  # src/aggre.cc
  # src/httpserver.cc
  # src/util.cc
  src/main.cc
)

target_link_libraries (${PROJECT_NAME} ${DPDK_LIBRARIES})
set_target_properties (${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS})