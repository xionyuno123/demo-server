cmake_minimum_required(VERSION 3.2)

project(test-pmd)

# These are command output of the pkg-config command. This is only used for recording.
# -include rte_config.h -march=native -I/home/djp/test/dpdk-install/include -I/usr/local/include -I/usr/include/libnl3
# -L/home/djp/test/dpdk-install/lib/x86_64-linux-gnu -Wl,--as-needed -lrte_node -lrte_graph -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_regexdev -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs

# Please specify the correct dpdk installation directory.
# The dpdk version must be 20.11.4
set(DPDK_INSTALL_PATH ${PROJECT_SOURCE_DIR}/deps/dpdk-install)

find_path(DPDK_INCLUDE_DIR rte_config.h
  HINTS ${DPDK_INSTALL_PATH}/include
  REQUIRED
  NO_DEFAULT_PATH)

# The following library list is reported by the pkg-config on installed libdpdk
set(components 
node graph bpf flow_classify pipeline table port fib ipsec vhost stack security sched reorder rib regexdev rawdev pdump power member lpm latencystats kni jobstats ip_frag gso gro eventdev efd distributor cryptodev compressdev cfgfile bitratestats bbdev acl timer hash metrics cmdline pci ethdev meter net mbuf mempool rcu ring eal telemetry kvargs net_bond net_i40e net_bnxt net_ixgbe net_dpaa bpf)

foreach(c ${components})
  find_library(DPDK_rte_${c}_LIBRARY rte_${c}
    HINTS ${DPDK_INSTALL_PATH}/lib/x86_64-linux-gnu
    REQUIRED
    NO_DEFAULT_PATH)
endforeach()

# /usr/local/lib
find_library(HTTPD microhttpd REQUIRED)
find_library(JSON jansson REQUIRED)

foreach(c ${components})
  list(APPEND check_LIBRARIES "${DPDK_rte_${c}_LIBRARY}")
endforeach()

list(APPEND check_LIBRARIES "${HTTPD}")
list(APPEND check_LIBRARIES "${JSON}")

# By marking the two variables as advanced, they will not appear in cmake output logs.
# mark_as_advanced(DPDK_INCLUDE_DIR ${check_LIBRARIES})

set(DPDK_LIBRARIES
    -Wl,--whole-archive ${check_LIBRARIES} -lpthread -lnuma -ldl -lm -Wl,--no-whole-archive)

# The cpp library
# add_library(cpp_lib
#   STATIC
#   cppsrc/fill_buf.cc
# )
# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3 -march=native")
# set_target_properties (cpp_lib PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS})
# target_link_libraries(cpp_lib -L${DPDK_INSTALL_PATH}/lib/x86_64-linux-gnu)
# target_link_libraries (cpp_lib ${DPDK_LIBRARIES})
# target_include_directories(cpp_lib PUBLIC ${DPDK_INCLUDE_DIR})


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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -include rte_config.h -march=native -w")
target_link_libraries(${PROJECT_NAME} -L${DPDK_INSTALL_PATH}/lib/x86_64-linux-gnu)
target_link_libraries(${PROJECT_NAME} -Wl,--whole-archive -l:librte_common_cpt.a -l:librte_common_dpaax.a -l:librte_common_iavf.a -l:librte_common_octeontx.a -l:librte_common_octeontx2.a -l:librte_common_sfc_efx.a -l:librte_bus_dpaa.a -l:librte_bus_fslmc.a -l:librte_bus_ifpga.a -l:librte_bus_pci.a -l:librte_bus_vdev.a -l:librte_bus_vmbus.a -l:librte_common_mlx5.a -l:librte_common_qat.a -l:librte_mempool_bucket.a -l:librte_mempool_dpaa.a -l:librte_mempool_dpaa2.a -l:librte_mempool_octeontx.a -l:librte_mempool_octeontx2.a -l:librte_mempool_ring.a -l:librte_mempool_stack.a -l:librte_net_af_packet.a -l:librte_net_ark.a -l:librte_net_atlantic.a -l:librte_net_avp.a -l:librte_net_axgbe.a -l:librte_net_bond.a -l:librte_net_bnx2x.a -l:librte_net_bnxt.a -l:librte_net_cxgbe.a -l:librte_net_dpaa.a -l:librte_net_dpaa2.a -l:librte_net_e1000.a -l:librte_net_ena.a -l:librte_net_enetc.a -l:librte_net_enic.a -l:librte_net_failsafe.a -l:librte_net_fm10k.a -l:librte_net_i40e.a -l:librte_net_hinic.a -l:librte_net_hns3.a -l:librte_net_iavf.a -l:librte_net_ice.a -l:librte_net_igc.a -l:librte_net_ixgbe.a -l:librte_net_kni.a -l:librte_net_liquidio.a -l:librte_net_memif.a -l:librte_net_mlx5.a -l:librte_net_netvsc.a -l:librte_net_nfp.a -l:librte_net_null.a -l:librte_net_octeontx.a -l:librte_net_octeontx2.a -l:librte_net_pfe.a -l:librte_net_qede.a -l:librte_net_ring.a -l:librte_net_sfc.a -l:librte_net_softnic.a -l:librte_net_tap.a -l:librte_net_thunderx.a -l:librte_net_txgbe.a -l:librte_net_vdev_netvsc.a -l:librte_net_vhost.a -l:librte_net_virtio.a -l:librte_net_vmxnet3.a -l:librte_raw_dpaa2_cmdif.a -l:librte_raw_dpaa2_qdma.a -l:librte_raw_ioat.a -l:librte_raw_ntb.a -l:librte_raw_octeontx2_dma.a -l:librte_raw_octeontx2_ep.a -l:librte_raw_skeleton.a -l:librte_crypto_bcmfs.a -l:librte_crypto_caam_jr.a -l:librte_crypto_ccp.a -l:librte_crypto_dpaa_sec.a -l:librte_crypto_dpaa2_sec.a -l:librte_crypto_nitrox.a -l:librte_crypto_null.a -l:librte_crypto_octeontx.a -l:librte_crypto_octeontx2.a -l:librte_crypto_openssl.a -l:librte_crypto_scheduler.a -l:librte_crypto_virtio.a -l:librte_compress_octeontx.a -l:librte_compress_zlib.a -l:librte_regex_mlx5.a -l:librte_regex_octeontx2.a -l:librte_vdpa_ifc.a -l:librte_vdpa_mlx5.a -l:librte_event_dlb.a -l:librte_event_dlb2.a -l:librte_event_dpaa.a -l:librte_event_dpaa2.a -l:librte_event_octeontx2.a -l:librte_event_opdl.a -l:librte_event_skeleton.a -l:librte_event_sw.a -l:librte_event_dsw.a -l:librte_event_octeontx.a -l:librte_baseband_null.a -l:librte_baseband_turbo_sw.a -l:librte_baseband_fpga_lte_fec.a -l:librte_baseband_fpga_5gnr_fec.a -l:librte_baseband_acc100.a -l:librte_node.a -l:librte_graph.a -l:librte_bpf.a -l:librte_flow_classify.a -l:librte_pipeline.a -l:librte_table.a -l:librte_port.a -l:librte_fib.a -l:librte_ipsec.a -l:librte_vhost.a -l:librte_stack.a -l:librte_security.a -l:librte_sched.a -l:librte_reorder.a -l:librte_rib.a -l:librte_regexdev.a -l:librte_rawdev.a -l:librte_pdump.a -l:librte_power.a -l:librte_member.a -l:librte_lpm.a -l:librte_latencystats.a -l:librte_kni.a -l:librte_jobstats.a -l:librte_ip_frag.a -l:librte_gso.a -l:librte_gro.a -l:librte_eventdev.a -l:librte_efd.a -l:librte_distributor.a -l:librte_cryptodev.a -l:librte_compressdev.a -l:librte_cfgfile.a -l:librte_bitratestats.a -l:librte_bbdev.a -l:librte_acl.a -l:librte_timer.a -l:librte_hash.a -l:librte_metrics.a -l:librte_cmdline.a -l:librte_pci.a -l:librte_ethdev.a -l:librte_meter.a -l:librte_net.a -l:librte_mbuf.a -l:librte_mempool.a -l:librte_rcu.a -l:librte_ring.a -l:librte_eal.a -l:librte_telemetry.a -l:librte_kvargs.a -Wl,--no-whole-archive)
target_link_libraries (${PROJECT_NAME} -Wl,--as-needed -lrte_node -lrte_graph -lrte_bpf -lrte_flow_classify -lrte_pipeline -lrte_table -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_stack -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_regexdev -lrte_rawdev -lrte_pdump -lrte_power -lrte_member -lrte_lpm -lrte_latencystats -lrte_kni -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -lrte_compressdev -lrte_cfgfile -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -pthread -lm -ldl -lnuma -ljansson -lmicrohttpd)
target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC 
  ${DPDK_INCLUDE_DIR} 
  ${PROJECT_SOURCE_DIR}/include
)