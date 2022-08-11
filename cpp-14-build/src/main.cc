#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include <signal.h>
#include <getopt.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <array>
#include <cstdint>
#include <set>

// #include "testpmd.h"

#define NB_MBUF   8192
#define MEMPOOL_CACHE_SIZE 256
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
#define MAX_PKT_BURST 32

#define RED "\033[31m"
#define RESET "\033[0m"
#define GREEN "\033[32m"

static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

static volatile bool force_quit = false;;

struct rte_mempool * l2fwd_pktmbuf_pool = NULL;

static volatile uint32_t port_mask = 0;

static void
check_all_ports_link_status(uint16_t port_num, uint8_t* all_ports_up)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 200 /* 9s (90 * 100ms) in total */
    uint16_t portid;
    uint8_t count, print_flag = 0;
    struct rte_eth_link link;

    printf("\nChecking link status\n");
    fflush(stdout);
    for (count = 0; count <= MAX_CHECK_TIME; count++) {
        if (force_quit)
            return;
        *all_ports_up = 1;
        for (portid = 0; portid < port_num; portid++) {
            if (force_quit)
                return;
            if ((port_mask & (1 << portid)) == 0)
                continue;
            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(portid, &link);
            /* print link status if flag set */
            if (print_flag == 1) {
                if (!link.link_status) {
                    printf("Port %d Link Down\n", portid);
                    *all_ports_up = 0;
                }
                continue;
            }
            /* clear all_ports_up flag if any link down */
            if (link.link_status == ETH_LINK_DOWN) {
                *all_ports_up = 0;
                break;
            }
        }
        /* after finally printing all link status, get out */
        if (print_flag == 1)
            break;

        if (*all_ports_up == 0) {
            rte_delay_ms(CHECK_INTERVAL);
        }

        /* set the print_flag if all ports up or timeout */
        if (*all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
            print_flag = 1;
        }
    }
}

static void
signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n",
               signum);
        force_quit = true;
    }
}

static int
lsi_event_callback(uint16_t port_id, enum rte_eth_event_type type, void *param,
            void *ret_param)
{
    struct rte_eth_link link;

    RTE_SET_USED(param);
    RTE_SET_USED(ret_param);

    // printf("\n\nIn registered callback...\n");
    // printf("Event type: %s\n", type == RTE_ETH_EVENT_INTR_LSC ? "LSC interrupt" : "unknown event");
    rte_eth_link_get_nowait(port_id, &link);
    if (link.link_status) {
        printf("Port %d Link Up - speed %u Mbps - %s\n\n",
                port_id, (unsigned)link.link_speed,
            (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                ("full-duplex") : ("half-duplex"));
        port_mask = port_mask | (1 << port_id);
    } else {
        port_mask = port_mask & (~(1 << port_id));

        if(!(port_mask & (1 << 1)) && 
           !(port_mask & (1 << 2)) && 
           !(port_mask & (1 << 3)) ) {
            printf( RED "FATAL:" RESET " All links are down! The connection is lost.\n");
        }
        else {
            printf("Port %d Link Down. Repairing... " GREEN "Repaired.\n\n" RESET, port_id);
        }
    }

    return 0;
}

class demo_option_paser {
public:
    demo_option_paser ()
    : _ingress_side_server_mac()
    , _ingress_mac()
    , _ingress_mac_val()
    , _ingress_side_port_id(-1)
    , _egress_side_server_mac()
    , _egress_mac()
    , _egress_mac_val()
    , _egress_side_port_id() {
    }

    bool parser_init(int argc, char **argv) {
        bool succeed = true;

        for(int i=1; i<argc; i=i+2) {
            std::string command(argv[i]);
            if(i+1>=argc) {
                break;
            }
            std::string command_val(argv[i+1]);

            if (command == "--ingress-side-server-mac") {
                // The command_val should be have this format:
                // AA:BB:CC:DD:EE:FF,11:22:33:44:55.
                auto mac_addr_vec = split(command_val, ',');
                for(auto& v : mac_addr_vec) {
                    auto mac_byte_val_vec = split(v, ':');
                    if(mac_byte_val_vec.size() != 6) {
                        _ingress_side_server_mac.clear();
                        _ingress_mac.clear();
                        break;
                    }
                    _ingress_side_server_mac.push_back(v);
                    std::array<uint8_t, 6> arr;
                    for(int i=0; i<6; i++) {
                        arr[i] = std::stoi(mac_byte_val_vec[i], nullptr, 16);
                    }
                    _ingress_mac.push_back(std::move(arr));
                }
            }
            else if(command == "--ingress-side-port-id") {
                // The command_val should be an integer
                _ingress_side_port_id = std::stoi(command_val);
                if(_ingress_side_port_id < 0) {
                        _ingress_side_port_id = -1;
                }
            }
            else if(command == "--egress-side-server-mac") {
                // The command_val should be have this format:
                // AA:BB:CC:DD:EE:FF,11:22:33:44:55.
                auto mac_addr_vec = split(command_val, ',');
                for(auto& v : mac_addr_vec) {
                    auto mac_byte_val_vec = split(v, ':');
                    if(mac_byte_val_vec.size() != 6) {
                        _egress_side_server_mac.clear();
                        _egress_mac.clear();
                        break;
                    }
                    _egress_side_server_mac.push_back(v);
                    std::array<uint8_t, 6> arr;
                    for(int i=0; i<6; i++) {
                        arr[i] = std::stoi(mac_byte_val_vec[i], nullptr, 16);
                    }
                    _egress_mac.push_back(std::move(arr));
                }
            }
            else if(command == "--egress-side-port-id") {
                // The command_val should have this format:
                // 12,2,3,4
                auto int_vec = split(command_val, ',');
                for(auto& i : int_vec) {
                    auto res = stoi(i);
                    if(res < 0) {
                        _egress_side_port_id.clear();
                        break;
                    }
                    _egress_side_port_id.push_back(res);
                }
            }
            else {
                break;
            }
        }

        if (_ingress_side_server_mac.size() == 0 || 
            _ingress_mac.size() == 0 || 
            _ingress_side_port_id == -1 || 
            _egress_side_server_mac.size() == 0 ||
            _egress_mac.size() == 0 ||
            _egress_side_port_id.size() == 0) {
            succeed = false;
        }

        if(succeed == false) {
            usage(argv[0]);
        }
        else {
            std::cout<<"Parsing succeed!"<<std::endl;

            std::cout<<"Ingress-side MAC addresses: ";
            for(auto& str : _ingress_side_server_mac) {
                std::cout<<str<<", ";
            }
            std::cout<<std::endl;

            std::cout<<"Ingress-side port id: "<<_ingress_side_port_id<<std::endl;

            std::cout<<"Egress-side MAC addresses: ";
            for(auto& str : _egress_side_server_mac) {
                std::cout<<str<<", ";
            }
            std::cout<<std::endl;

            std::cout<<"Egress-side port ids: ";
            for(auto id : _egress_side_port_id) {
                std::cout<<id<<", ";
            }
            std::cout<<std::endl;

            for(auto& arr : _ingress_mac) {
                for(auto v : arr) {
                    std::cout << ((int)v) << ", ";
                }
                std::cout<<std::endl;
            }

            for(auto& arr : _egress_mac) {
                for(auto v : arr) {
                    std::cout << ((int)v) << ", ";
                }
                std::cout<<std::endl;
            }

            for(auto& arr : _ingress_mac) {
                    uint64_t* mac_val = reinterpret_cast<uint64_t*>(arr.data());
                    _ingress_mac_val.push_back(*mac_val &  0x0000FFFFFFFFFFFF);
            }

            for(auto& arr : _egress_mac) {
                    uint64_t* mac_val = reinterpret_cast<uint64_t*>(arr.data());
                    _egress_mac_val.push_back(*mac_val &  0x0000FFFFFFFFFFFF);
            }
        }

        return succeed;
    }

public:
    inline int ingress_side_port_id() {
            return _ingress_side_port_id;
    }

    inline int egress_side_port_id_count() {
            return _egress_side_port_id.size();
    }
    inline int egress_side_port_id(int index) {
            return _egress_side_port_id.at(index);
    }

    inline int ingress_server_index(uint64_t mac_val) {
            int res = 0;
            for(auto val : _ingress_mac_val) {
                if(mac_val == val) {
                    return res;
                }
                res += 1;
            }
            return res;
    }
    inline int ingress_server_count() {
            return _ingress_mac_val.size();
    }

    inline uint64_t egress_mac_val(int index) {
            return _egress_mac_val.at(index);
    }
    inline int egress_server_count() {
        return _egress_mac_val.size();
    }

private:
     void usage(const char *prgname) {
        printf("%s [EAL options] -- --ingress-side-server-mac MACLIST --ingress-side-port-id PORTNUM \n"
               "--egress-side-server-mac MACLIST --egress-side-port-id PORTNUMLIST\n"
               "  --ingress-side-server-mac MACLIS: Mac addresses of ingress-side servers, separated by colon\n"
               "  --ingress-side-port-id PORTNUM: Ingress-side port number\n"
               "  --egress-side-server-mac MACLIS: Mac addresses of egress-side servers, separated by colon\n"
               "  --egress-side-port-id PORTNUMLIST: A list of egress-side port number, separated by colon\n",
            prgname);
    }

    std::vector<std::string> split(std::string& str, char delimiter) {
        std::vector<std::string> vec(0);
        std::stringstream ss(str);
        std::string item;
        while(std::getline(ss, item, delimiter)){
            if(!item.empty()) {
                vec.push_back(item);
            }
        }
        return vec;    
    }

private:
    std::vector<std::string> _ingress_side_server_mac;
    std::vector<std::array<uint8_t, 6>> _ingress_mac;
    std::vector<uint64_t> _ingress_mac_val;
    int _ingress_side_port_id;
    std::vector<std::string> _egress_side_server_mac;
    std::vector<std::array<uint8_t, 6>> _egress_mac;
    std::vector<uint64_t> _egress_mac_val;
    std::vector<int> _egress_side_port_id;
};

static inline uint64_t source_mac_of_rte_mbuf(struct rte_mbuf* mbuf) {
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    uint64_t* src_mac_val = reinterpret_cast<uint64_t*>(&eth->s_addr.addr_bytes[0]);
    return (*src_mac_val) & 0x0000FFFFFFFFFFFF;
}

static inline void update_source_mac(struct rte_mbuf* mbuf, uint8_t server_index,
                                     uint64_t server_counter) {
    // After update, the source mac of an mbuf becomes:
    // XX:YY:YY:YY:YY:YY
    // YY is the server counter, total of 40 bits, representing 2^40 packets
    // XX is the server index
    struct rte_ether_hdr tmp_eth;
    void* tmp = &tmp_eth.s_addr.addr_bytes[0];
    *(reinterpret_cast<uint64_t*>(tmp)) =
            ((uint64_t)server_index << 40) + (server_counter & 0x000000FFFFFFFFFF);
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    eth->s_addr = tmp_eth.s_addr;
}

static inline uint64_t server_counter_of_source_mac(struct rte_mbuf* mbuf) {
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    uint64_t* src_mac_val = reinterpret_cast<uint64_t*>(&eth->s_addr.addr_bytes[0]);
    return (*src_mac_val) & 0x000000FFFFFFFFFF;
}

static inline uint8_t server_index_of_source_mac(struct rte_mbuf* mbuf) {
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    return eth->s_addr.addr_bytes[5];
}

static inline void restore_source_mac(struct rte_mbuf* mbuf, uint64_t mac_val) {
    struct rte_ether_hdr tmp_eth;
    void* tmp = &tmp_eth.s_addr.addr_bytes[0];
    *(reinterpret_cast<uint64_t*>(tmp)) = mac_val;
    struct rte_ether_hdr *eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    eth->s_addr = tmp_eth.s_addr;
}

void ingress_pipeline(demo_option_paser& opt_parser,
                      std::vector<uint64_t>& counters,
                      std::vector<uint64_t>& tmp_counters,
                      std::vector<std::vector<uint64_t>>& recorder) {
    // Prerequisite:
    // counters.size() == opt_parser.ingress_side_server_mac.size()
    // tmp_counters.size() == counters.size()
    // recorder.size() == MAX_PKT_BURST
    // recorder.at(i).size == opt_parser.ingress_side_server_mac.size()

    struct rte_mbuf *original_pkts[MAX_PKT_BURST];
    struct rte_mbuf *filtered_pkts[MAX_PKT_BURST];
    struct rte_mbuf *cloned_pkts[MAX_PKT_BURST];

    for(int i=0; i<(int)counters.size(); i++) {
        tmp_counters.at(i) = counters.at(i);
    }

    uint16_t nb_rx = rte_eth_rx_burst(opt_parser.ingress_side_port_id(), 0,
                       original_pkts, MAX_PKT_BURST);
    if(unlikely(nb_rx == 0)) {
        return;
    }

    int filtered_size = 0;
    for(int i=0; i<nb_rx; i++) {
        struct rte_mbuf* pkt = original_pkts[i];

        uint64_t mac_val = source_mac_of_rte_mbuf(pkt);
        int server_index = opt_parser.ingress_server_index(mac_val);
        if(server_index == opt_parser.ingress_server_count()) {
            // The mac address does not match any of the ingress side server mac
            rte_pktmbuf_free(pkt);
            continue;
        }

        tmp_counters.at(server_index) += 1;

        update_source_mac(pkt, server_index, tmp_counters.at(server_index));

        for(int j=0; j<(int)tmp_counters.size(); j++) {
            recorder.at(filtered_size).at(j) = tmp_counters.at(j);
        }

        filtered_pkts[filtered_size] = pkt;
        filtered_size += 1;
    }

    int egress_port_count = opt_parser.egress_side_port_id_count();
    uint16_t max_nb_tx = 0;
    for(int id=0; id<egress_port_count; id++) {
        int egress_port_id = opt_parser.egress_side_port_id(id);
        if(!(port_mask & (1 << egress_port_id))) {
            continue;
        }

        bool clone_succeed = true;
        for(int i=0; i<filtered_size; i++) {
            cloned_pkts[i] = rte_pktmbuf_clone(filtered_pkts[i], l2fwd_pktmbuf_pool);
            if(cloned_pkts[i] == nullptr) {
                clone_succeed = false;
            }
        }
        if(clone_succeed == false) {
            for(int i=0; i<filtered_size; i++) {
                if(cloned_pkts[i] != nullptr) {
                    rte_pktmbuf_free(cloned_pkts[i]);
                }
            }
            continue;
        }

        uint16_t nb_tx = rte_eth_tx_burst(egress_port_id, 0, cloned_pkts, filtered_size);
        if(nb_tx < filtered_size) {
            for(int i = nb_tx; i<filtered_size; i++) {
                rte_pktmbuf_free(cloned_pkts[i]);
            }
        }

        if(nb_tx > max_nb_tx) {
            max_nb_tx = nb_tx;
        }
    }

    if(max_nb_tx != 0) {
        for(int i=0;  i<(int)counters.size(); i++) {
            counters.at(i) = recorder.at(max_nb_tx-1).at(i);
        }
    }

    for(int i=0; i<filtered_size; i++) {
        rte_pktmbuf_free(filtered_pkts[i]);
    }
}

void egress_pipeline(int egress_port_id,
                     demo_option_paser& opt_parser,
                     std::vector<uint64_t>& counters) {

    // Prerequisite:
    // counters.size() == egress_side_server_mac.size();

    struct rte_mbuf* original_pkts[MAX_PKT_BURST];
    struct rte_mbuf* filtered_pkts[MAX_PKT_BURST];

    uint16_t nb_rx = rte_eth_rx_burst(egress_port_id, 0,
                       original_pkts, MAX_PKT_BURST);

    int filtered_size = 0;
    for(int i=0; i<nb_rx; i++) {
        struct rte_mbuf* pkt = original_pkts[i];
        uint64_t server_counter = server_counter_of_source_mac(pkt);
        uint8_t server_index = server_index_of_source_mac(pkt);

        if(server_index >= opt_parser.egress_server_count() ||
           counters.at(server_index) >= server_counter ) {
            rte_pktmbuf_free(pkt);
            continue;
        }

        counters.at(server_index) = server_counter;

        restore_source_mac(pkt, opt_parser.egress_mac_val(server_index));
        
        filtered_pkts[filtered_size] = pkt;
        filtered_size += 1;
    }

    uint16_t nb_tx = rte_eth_tx_burst(opt_parser.ingress_side_port_id(), 0, 
                                      filtered_pkts, filtered_size);
    if(nb_tx < filtered_size) {
        for(int i = nb_tx; i<filtered_size; i++) {
            rte_pktmbuf_free(filtered_pkts[i]);
        }
    }
}

int main(int argc, char **argv) {
    struct rte_eth_conf port_conf;

    port_conf.rxmode.split_hdr_size = 0;
    // port_conf.rxmode.header_split   = 0; /**< Header Split disabled */
    // port_conf.rxmode.hw_ip_checksum = 0; /**< IP checksum offload disabled */
    // port_conf.rxmode.hw_vlan_filter = 0; /**< VLAN filtering disabled */
    // port_conf.rxmode.jumbo_frame    = 0; /**< Jumbo Frame Support disabled */
    // port_conf.rxmode.hw_strip_crc   = 1; /**< CRC stripped by hardware */

    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;

    port_conf.intr_conf.lsc = 1;

    /* init EAL */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
    argc -= ret;
    argv += ret;

    /* set up the signal handler */
    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    demo_option_paser opt_parser;
    bool succeed = opt_parser.parser_init(argc, argv);
    if(succeed == false) {
        rte_exit(EXIT_FAILURE, "Invalid demo arguments\n");
    }

    std::cout<<"The process is running on socket "<<rte_socket_id()<<std::endl;

    l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
        MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());
    if (l2fwd_pktmbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
    else
        std::cout << "Finish creating packet memory buffer pool." <<std::endl;

    int max_port_id = opt_parser.ingress_side_port_id();
    std::set<int> port_id_holder;
    port_id_holder.insert(max_port_id);
    int count = opt_parser.egress_side_port_id_count();
    for(int i=0; i<count; i++) {
        auto res = port_id_holder.insert(opt_parser.egress_side_port_id(i));
        if(res.second == false) {
            rte_exit(EXIT_FAILURE, "Invalid ingress/egress port id.\n");
        }
        if(opt_parser.egress_side_port_id(i) > max_port_id) {
            max_port_id = opt_parser.egress_side_port_id(i);
        }
    }

    uint16_t nb_ports = 1;// rte_eth_dev_count();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");
    else
        std::cout<<(int)nb_ports<<" port available on this machine."<<std::endl;

    if(max_port_id >= nb_ports) {
        rte_exit(EXIT_FAILURE, "Invalid ingress/egress port id.\n");
    }

    port_mask = 0;
    for(auto port_id : port_id_holder) {
        port_mask = port_mask | (1 << port_id);
    }

    for(auto id : port_id_holder) {
        uint16_t portid = id;

        /* init port */
        printf("Initializing port %u... ", portid);
        fflush(stdout);
        ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
                  ret, portid);

        ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
                                       &nb_txd);
        if (ret < 0)
            rte_exit(EXIT_FAILURE,
                 "Cannot adjust number of descriptors: err=%d, port=%u\n",
                 ret, portid);

        /* init one RX queue */
        fflush(stdout);
        ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
                         rte_eth_dev_socket_id(portid),
                         NULL,
                         l2fwd_pktmbuf_pool);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
                  ret, portid);

        /* init one TX queue on each port */
        fflush(stdout);
        ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
                rte_eth_dev_socket_id(portid),
                NULL);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
                ret, portid);

        /* Start device */
        ret = rte_eth_dev_start(portid);
        if (ret < 0)
            rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
                  ret, portid);

        printf("done. \n");
        rte_eth_promiscuous_enable(portid);
    }

    uint8_t all_ports_up = 0;
    check_all_ports_link_status(nb_ports, &all_ports_up);
    if(all_ports_up == 0) {
        rte_exit(EXIT_FAILURE, "Some links are down, exit.\n");
    }
    else {
        rte_delay_ms(1000);
        printf("Done!\n");
    }

    for(auto id : port_id_holder) {
        uint16_t portid = id;

         /* register lsi interrupt callback, need to be after
         * rte_eth_dev_configure(). if (intr_conf.lsc == 0), no
         * lsc interrupt will be present, and below callback to
         * be registered will never be called.
         */
        rte_eth_dev_callback_register(portid,
            RTE_ETH_EVENT_INTR_LSC, lsi_event_callback, NULL);
    }

    std::vector<uint64_t> ingress_counters(opt_parser.ingress_server_count(), 0);
    std::vector<uint64_t> ingress_tmp_counters(opt_parser.ingress_server_count(), 0);
    std::vector<std::vector<uint64_t>> ingress_recorder;
    for(int i=0; i<MAX_PKT_BURST; i++) {
        ingress_recorder.push_back(
            std::vector<uint64_t>(opt_parser.ingress_server_count(), 0));
    }

    std::vector<uint64_t> egress_counters(opt_parser.egress_server_count(), 0);
    
    while(!force_quit) {
        ingress_pipeline(opt_parser,
                         ingress_counters,
                         ingress_tmp_counters,
                         ingress_recorder);
        
        for(int i=0; i<opt_parser.egress_side_port_id_count(); i++) {
            int egress_port_id = opt_parser.egress_side_port_id(i);
            if(port_mask & (1 << egress_port_id)) {
                egress_pipeline(egress_port_id, opt_parser, egress_counters);
            }
            else {
                continue;
            }
        }
    }
}