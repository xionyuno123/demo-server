 // *.h file
 // ...
#include <rte_mbuf.h>

 #ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif

EXTERNC void fill_buf(char* buf, int buf_len);


EXTERNC inline void construct_pkt(struct rte_mbuf** mbufs);

