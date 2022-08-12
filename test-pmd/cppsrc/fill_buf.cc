#include "fill_buf.h"
#include <rte_mbuf.h>
#include <string>
#include <cstring>
#include <iostream>

void fill_buf(char* buf, int buf_len) {
    std::string content = "wtf?";

    if(buf_len >= content.size() + 1) {
        std::memcpy(buf, content.c_str(), content.size()+1);
    }
}

void construct_pkt(struct rte_mbuf* mbuf) {
    if(mbuf != NULL) {
        rte_mbuf_sanity_check(mbuf, 1);
    }
    else {
        std::cout<<"input packet is a nullptr"<<std::endl;
    }
}