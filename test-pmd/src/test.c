#include <stdio.h>
__uint64_t video_map[1024];

static inline void  set_port_map(__uint16_t pos){
    size_t index=pos>>6;
    if((video_map[index] & (0x1l << (pos&63)))!=0){

    }
    else{
        printf("%d ",pos);
        video_map[index]|=(0x1l << (pos & 63));
    }
}

static __uint8_t table[256] = 
    { 
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 
}; 
static inline __uint16_t bits_counts(__uint64_t value){
        return 
         table[value & 0xff] 
        +table[(value>>8) & 0xff]
        +table[(value>>16) & 0xff]
        +table[(value>>24) & 0xff]
        +table[(value>>32) & 0xff]
        +table[(value>>40) & 0xff]
        +table[(value>>48) & 0xff]
        +table[(value>>56) & 0xff];
}


void main(){
    int i=0;
    for (i;i<__UINT16_MAX__; ++i){
        set_port_map(i);
    }

    
    
    __uint64_t count=0;
    for(i=0;i<1024;++i){
            count+=bits_counts(video_map[i]);
    }

    for (i;i<__UINT16_MAX__; ++i){
        set_port_map(i);
    }
    
    printf("count=%ld \n",count);
}