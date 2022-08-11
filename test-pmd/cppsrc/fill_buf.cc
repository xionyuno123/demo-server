#include "fill_buf.h"
#include <rte_mbuf.h>
#include <string>
#include <cstring>

void fill_buf(char* buf, int buf_len) {
    std::string content = "wtf?";

    if(buf_len >= content.size() + 1) {
        std::memcpy(buf, content.c_str(), content.size()+1);
    }
}

inline void construct_pkt(struct rte_mbuf** mbuf) {

}