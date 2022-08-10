/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/queue.h>
#include <sys/stat.h>

#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_string_fns.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_net.h>
#include <rte_flow.h>
#include <rte_jhash.h>
#include "testpmd.h"


#define HASH_INITVAL 0x7135efee
volatile uint16_t udp_dst_port=5002;
uint64_t video_map[1024];
uint64_t video_rx_pkts[RTE_MAX_ETHPORTS];
uint64_t video_rx_bytes[RTE_MAX_ETHPORTS];
rte_spinlock_t video_spinlock;

/*
    Return
        >=0 SUCCESS, return the position of aggre_streams_stats
        -1 Failed to insert (FULL or EXSITING)
*/
static int insert_hash_table(struct hash_key *key){
    int di;
    int pos=0;
    for(di=0;di<RTE_MAX_STREAMS;di++){
        uint32_t hash_value=rte_jhash(key,sizeof(struct hash_key),HASH_INITVAL);
        pos=(hash_value+di)%RTE_MAX_STREAMS;
        if(rte_atomic16_cmpset(&(aggre_streams_stats[pos].status),false,true)!=0){
            aggre_streams_stats[pos].hash_value=hash_value;
            memcpy(&(aggre_streams_stats[pos].key),key,sizeof(struct hash_key));
            
            return pos;
        }
        else if(aggre_streams_stats[pos].hash_value==hash_value){
            return pos;
        }
        else{
            pos=-1;
        }
    }
    
    return pos;
}




static inline void  set_port_map(uint16_t pos){
    size_t index=pos>>6;
    video_map[index]|=(0x1l << (pos & 63));
}



/*
 * Received a burst of packets.
 */
static void
aggregation_burst_receive(struct fwd_stream *fs)
{
	struct rte_mbuf  *pkts_burst[MAX_PKT_BURST];
	uint16_t nb_rx;
	uint16_t i;
	uint64_t start_tsc = 0;

	

	/*
	 * Receive a burst of packets.
	 */
	nb_rx = rte_eth_rx_burst(fs->rx_port, fs->rx_queue, pkts_burst,
				 nb_pkt_per_burst);
	
	if (unlikely(nb_rx == 0))
		return;


	for (i = 0; i < nb_rx; i++)
	{
        struct rte_ether_hdr* ether_hdr=rte_pktmbuf_mtod_offset(pkts_burst[i],struct rte_ether_hdr*,0);
        struct rte_ipv4_hdr* ipv4_hdr=rte_pktmbuf_mtod_offset(pkts_burst[i],struct rte_ipv4_hdr*,sizeof(struct rte_ether_hdr));
        struct rte_udp_hdr* udp_hdr=rte_pktmbuf_mtod_offset(pkts_burst[i],struct rte_udp_hdr*,sizeof(struct rte_ether_hdr)+sizeof(struct rte_ipv4_hdr));
        struct packet_marker* maker=rte_pktmbuf_mtod_offset(pkts_burst[i],struct packet_marker*,sizeof(struct rte_ether_hdr)+sizeof(struct rte_ipv4_hdr)+sizeof(struct rte_udp_hdr));
        uint16_t pkt_size=pkts_burst[i]->data_len;
        uint64_t max_sq=rte_be_to_cpu_64(maker->sq);

        if(maker->magic_num == rte_cpu_to_be_64(8386112020297315432l)){
            struct hash_key key;
            key.dst_ip_addr=ipv4_hdr->dst_addr;
            key.src_ip_addr=ipv4_hdr->src_addr;
            key.dst_port=udp_hdr->dst_port;
            key.src_port=udp_hdr->src_port;
            memcpy(&(key.dst_mac_addr),&(ether_hdr->d_addr),sizeof(struct rte_ether_addr));
            memcpy(&(key.src_mac_addr),&(ether_hdr->s_addr),sizeof(struct rte_ether_addr));

            int pos=insert_hash_table(&key);
            if(pos==-1){
                perror("Received stream exceed than the RTE_MAX_STREAMS\n");
                continue;
            }
            rte_spinlock_lock(&(aggre_streams_stats[pos].spinlock));
            aggre_streams_stats[pos].max_sq=RTE_MAX(aggre_streams_stats[pos].max_sq,max_sq);
            aggre_streams_stats[pos].rx_pkts++;
            aggre_streams_stats[pos].pkt_sz=pkt_size;
            rte_spinlock_unlock(&(aggre_streams_stats[pos].spinlock));
            rte_pktmbuf_free(pkts_burst[i]);
            continue;
        }        
        /* 
            forward stream
         */
        else{
            
            /* 
                统计条数和总流量
            */
            uint16_t dst_port=rte_be_to_cpu_16(udp_hdr->dst_port);
            uint16_t src_port=rte_be_to_cpu_16(udp_hdr->src_port);
            rte_spinlock_lock(&(video_spinlock));
            set_port_map(dst_port);
            rte_spinlock_unlock(&(video_spinlock));
            video_rx_pkts[fs->rx_port]++;
            video_rx_bytes[fs->rx_port]+=pkts_burst[i]->pkt_len;
            

            if (dst_port==udp_dst_port){
                ipv4_hdr->dst_addr=rte_cpu_to_be_32(10<<24|50<<16|220<<8|111);
                struct hash_key key;
                struct rte_ether_addr dst_addr={
                    0x08,0x68,0x8d,0x61,0x7c,0x24
                };
                memcpy(&(key.src_mac_addr),&(ether_hdr->s_addr),sizeof(struct rte_ether_addr));
                memcpy(&(ether_hdr->s_addr),&(ether_hdr->d_addr),sizeof(struct rte_ether_addr));
                memcpy(&(key.dst_mac_addr),&(ether_hdr->d_addr),sizeof(struct rte_ether_addr));
                memcpy(&(ether_hdr->d_addr),&(dst_addr),sizeof(struct rte_ether_addr));
                /* 
                forward stream
                */
                 int retry=0;
                 uint16_t nb_pkts=0;
                /* 
                    重新计算下ip checksum
                 */
                uint16_t *ptr16;
	            uint32_t ip_cksum;
                ptr16 = (unaligned_uint16_t*) ipv4_hdr;
	            ip_cksum = 0;
	            ip_cksum += ptr16[0]; ip_cksum += ptr16[1];
	            ip_cksum += ptr16[2]; ip_cksum += ptr16[3];
	            ip_cksum += ptr16[4];
	            ip_cksum += ptr16[6]; ip_cksum += ptr16[7];
	            ip_cksum += ptr16[8]; ip_cksum += ptr16[9];
                ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) + (ip_cksum & 0x0000FFFF);
	           if (ip_cksum > 65535)
		           ip_cksum -= 65535;
	           ip_cksum = (~ip_cksum) & 0x0000FFFF;
	           if (ip_cksum == 0)
		           ip_cksum = 0xFFFF;
	            ipv4_hdr->hdr_checksum = (uint16_t) ip_cksum;
                udp_hdr->dgram_cksum=0;


                do{
                     nb_pkts=rte_eth_tx_burst(fs->rx_port,fs->tx_queue,(pkts_burst+i),1);
                } while(nb_pkts!=1&&retry++<10);
            
                 /* 
                    failed to retry
                */
                    if(unlikely(nb_pkts!=1)){
                        rte_pktmbuf_free(pkts_burst[i]);
                    }
            }
            else
            rte_pktmbuf_free(pkts_burst[i]);
        }
        
        
    }

	
}


static void aggregation_begin(uint16_t lc){
    memset(aggre_streams_stats,0,sizeof(struct hash_data)*RTE_MAX_STREAMS);
}

static void aggregation_end(uint16_t lc){

}

struct fwd_engine aggregation_engine = {
	.fwd_mode_name  = "aggre",
	.port_fwd_begin = aggregation_begin,
	.port_fwd_end   = NULL,
	.packet_fwd     = aggregation_burst_receive,
};
